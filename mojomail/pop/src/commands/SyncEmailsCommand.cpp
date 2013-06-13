// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// LICENSE@@@

#include <ctime>
#include "commands/SyncEmailsCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/UidCacheAdapter.h"
#include "data/PopEmailAdapter.h"
#include "PopConfig.h"
#include "PopDefs.h"

const int SyncEmailsCommand::LOAD_EMAIL_BATCH_SIZE 	= 10;
const int SyncEmailsCommand::SAVE_EMAIL_BATCH_SIZE 	= 5;
const int SyncEmailsCommand::SECONDS_IN_A_DAY 		= 24 * 60 * 60;

SyncEmailsCommand::SyncEmailsCommand(PopSession& session, const MojObject& folderId, boost::shared_ptr<UidMap>& uidMap)
: HandleRequestCommand(session, "Sync emails"),
  m_account(session.GetAccount()),
  m_folderId(folderId),
  m_state(State_ReconcileEmails),
  m_orgState(State_None),
  m_visitedEmailHeadersCount(0),
  m_persistEmails(new PopEmail::PopEmailPtrVector()),
  m_latestEmailTimestamp(MojInt64Min),
  m_prevEmailTimestamp(MojInt64Max),
  m_wasMsgsOrdered(true),
  m_lookbackCount(0),
  m_totalMessageCount(0),
  m_readyResponseSlot(this, &SyncEmailsCommand::SyncSessionReadyResponse),
  m_reconcileResponseSlot(this, &SyncEmailsCommand::ReconcileEmailsResponse),
  m_deleteLocalEmailsResponseSlot(this, &SyncEmailsCommand::DeleteLocalEmailsResponse),
  m_getEmailHeaderResponseSlot(this, &SyncEmailsCommand::GetEmailHeaderResponse),
  m_saveEmailsResponseSlot(this, &SyncEmailsCommand::SaveEmailsResponse),
  m_trimEmailsResponseSlot(this, &SyncEmailsCommand::TrimEmailsResponse),
  m_deleteServerEmailsResponseSlot(this, &SyncEmailsCommand::DeleteServerEmailsResponse),
  m_loadUidCacheResponseSlot(this, &SyncEmailsCommand::LoadUidCacheResponse),
  m_saveUidCacheResponseSlot(this, &SyncEmailsCommand::SaveUidCacheResponse),
  m_handleRequestResponseSlot(this, &SyncEmailsCommand::HandleRequestResponse)
{
	m_syncWindow = m_account->GetSyncWindowDays();
	m_cutOffTime = GetCutOffTime(m_syncWindow);
	m_uidMap = uidMap;
}

SyncEmailsCommand::~SyncEmailsCommand()
{
}

void SyncEmailsCommand::RunImpl()
{
	m_session.GetSyncSession()->RegisterCommand(this, m_readyResponseSlot);
}

MojErr SyncEmailsCommand::SyncSessionReadyResponse()
{
	MojLogInfo(m_log, "Syncing folder");
	m_session.SetState(PopSession::State_SyncingEmails);
	CheckState();

	return MojErrNone;
}

void SyncEmailsCommand::HandleNextRequest() {
	Request::RequestPtr request = m_requests.front();
	m_requests.pop();

	// TODO: should use PopCommandResult's done slot instead of request object's done slot. 
	// TODO: should remove request object's done slot.
	m_handleRequestResponseSlot.cancel();
	request->ConnectDoneSlot(m_handleRequestResponseSlot);
	MojLogInfo(m_log, "======== Handling on-demand %s request", (request->GetRequestType() == Request::Request_Fetch_Email ? "fetch email" : ""));
	m_fetchEmailCommand.reset(new FetchEmailCommand(m_session, request));
	m_fetchEmailCommand->Run();
}

MojErr SyncEmailsCommand::HandleRequestResponse() {
	if (m_requests.empty()) {
		// no more on-demand request to handle, then switch back to the original
		// state and continue to run it inside this state machine.
		m_state = m_orgState;
		m_orgState = State_None;
	}

	CheckState();
	return MojErrNone;
}

MojInt64 SyncEmailsCommand::GetCutOffTime(int lookbackDays)
{
	time_t now = time(NULL);
	tm* gmtm = gmtime(&now);
	time_t nowInGmt = mktime(gmtm);
	MojInt64 lookBackTime = nowInGmt - lookbackDays * SECONDS_IN_A_DAY;
	if (lookbackDays == 0) {
		// This is a sync all case.  Then can set the lookback time to 0 so that
		// all emails' timestamp will be considered to be greater than this value,
		// so they will be included in the folder sync.
		lookBackTime = 0;
	} else {
		lookBackTime *= 1000;
	}
	MojLogInfo(m_log, "************ local=%d, gmt=%d, syncwindow=%d, lookbacktime=%s", (int)now, (int)nowInGmt, m_syncWindow, AsJsonString(lookBackTime).c_str());
	return MojInt64(lookBackTime);
}

void SyncEmailsCommand::ReconcileEmails()
{
	m_commandResult.reset(new PopCommandResult(m_reconcileResponseSlot));

	m_reconcileEmailsCommand.reset(new ReconcileEmailsCommand(m_session,
			m_folderId,
			m_cutOffTime,
			m_latestEmailTimestamp,
			m_totalMessageCount,
			m_reconcileEmails,
			m_uidCache,
			m_expiredEmailIds,
			m_serverDeletedEmailIds,
			m_localDeletedEmailUids));
	m_reconcileEmailsCommand->SetResult(m_commandResult);
	m_reconcileEmailsCommand->Run();
}

MojErr SyncEmailsCommand::ReconcileEmailsResponse()
{
	try {
		m_commandResult->CheckException();
		m_state = State_DeleteLocalEmails;
		CheckState();
	} catch (const std::exception& ex) {
		// report error and finish command
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception ", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void SyncEmailsCommand::DeleteLocalEmails()
{
	m_commandResult.reset(new PopCommandResult(m_deleteLocalEmailsResponseSlot));

	m_deleteLocalEmailsCommand.reset(new DeleteLocalEmailsCommand(m_session,
			m_serverDeletedEmailIds,
			m_expiredEmailIds));
	m_deleteLocalEmailsCommand->SetResult(m_commandResult);
	m_deleteLocalEmailsCommand->Run();
}

MojErr SyncEmailsCommand::DeleteLocalEmailsResponse()
{
	// clear deleted email IDs lists to free up memory
	m_serverDeletedEmailIds.clear();
	m_expiredEmailIds.clear();

	// change to next state and run
	m_state = State_GetNextMessageToDownloadHeader;
	CheckState();

	return MojErrNone;
}

void SyncEmailsCommand::GetNextMessageToDownloadHeader()
{
	while (!m_reconcileEmails.empty()) {
		m_reconcileInfo = m_reconcileEmails.front();
		m_reconcileEmails.pop();

		if (m_reconcileInfo->GetStatus() == ReconcileEmailsCommand::Status_Fetch_Header) {
			m_state = State_DownloadEmailHeader;
		} else {
			CheckDownloadState(m_reconcileInfo->GetTimestamp());
		}

		switch (m_state) {
		case State_DownloadEmailHeader:
		case State_PersistEmails:
		case State_TrimEmails:
			CheckState();
			return;
		default:
			continue;
		}
	}

	if (m_persistEmails->size() > 0) {
		m_state = State_PersistEmails;
	} else {
		m_state = State_TrimEmails;
	}

	CheckState();
}

void SyncEmailsCommand::CheckDownloadState(const MojInt64& timestamp)
{
	// emails should be ordered in reverse chronological order.  so previous
	// emails should be the latest and have date greater than the later ones
	if (m_wasMsgsOrdered && timestamp > m_prevEmailTimestamp + PopConfig::ALLOWABLE_TIME_DISCREPANCY && m_prevEmailTimestamp != MojInt64Max) {
		// If this email is newer than the last, then the list is not in reverse
		// chronological order.
		m_wasMsgsOrdered = false;
	}

	// update previous email's timestamp slot and calculate the look back emails count
	m_prevEmailTimestamp = timestamp;
	if (ShouldIncludeInFolder(timestamp)) {
		m_lookbackCount = 0;
	} else {
		 m_lookbackCount++;
	}

	// stop downloading headers if we have seen enough continuous emails whose timestamp is
	// beyond the sync window.  We will check twice the amount of emails
	// if the list of email returned are not in chronological order.
	if ((m_wasMsgsOrdered && m_lookbackCount >= PopConfig::SYNC_BACK_EMAIL_COUNT)
		|| (!m_wasMsgsOrdered && m_lookbackCount >= 2 * PopConfig::SYNC_BACK_EMAIL_COUNT)) {
		m_state = m_persistEmails->size() > 0 ? State_PersistEmails : State_TrimEmails;

		// clear the message queue for downloading header since we don't need to
		// proceed any more.
		while (!m_reconcileEmails.empty()) {
			m_reconcileEmails.pop();
		}
	}
}

void SyncEmailsCommand::FetchEmailHeader(const ReconcileEmailsCommand::ReconcileInfoPtr& info)
{
	UidMap::MessageInfo msgInfo = info->GetMessageInfo();
	int msgNum = msgInfo.GetMessageNumber();
	std::string uid = msgInfo.GetUid();
	PopEmail::PopEmailPtr email(new PopEmail());

	MojLogInfo(m_log, "Downloading email header using message number %d, uid %s", msgNum, uid.c_str());
	// Push email pointer into persist email list first.  If the email's timestamp
	// is beyond sync window after header is downloaded, this email will be popped
	// back.
	m_persistEmails->push_back(email);

	m_getEmailHeaderResponseSlot.cancel();
	m_downloadHeaderCommand.reset(new DownloadEmailHeaderCommand(m_session, msgNum, email, m_getEmailHeaderResponseSlot));
	m_downloadHeaderCommand->Run();
}

MojErr SyncEmailsCommand::GetEmailHeaderResponse(bool failed)
{
	m_visitedEmailHeadersCount++;

	MojRefCountedPtr<PopCommandResult> result = m_downloadHeaderCommand->GetResult();
	try {
		// check error from DownloadEmailHeaderCommand
		result->CheckException();

		PopEmail::PopEmailPtr popEmail = m_persistEmails->back();

		MojInt64 msgTimestamp = popEmail->GetDateReceived();
		std::string uidStr = m_reconcileInfo->GetMessageInfo().GetUid();


		if (ShouldIncludeInFolder(msgTimestamp)) {
			MojLogInfo(m_log, "Adding new email to database");
			// set folder ID to email object
			popEmail->SetFolderId(m_folderId);
			// Set UID to the pop email
			popEmail->SetServerUID(uidStr);
			// Set email to be unread
			popEmail->SetRead(false);

			// increment the total number of emails in the inbox so that we
			// can determine whether we need to trim the inbox emails or not.
			m_totalMessageCount++;
		} else {
			MojLogInfo(m_log, "Putting email to old emails cache");

			// remove email from persist email list
			m_persistEmails->pop_back();
			// add emails beyond sync window into
			m_uidCache.GetOldEmailsCache().AddToCache(uidStr, msgTimestamp);
			m_uidCache.GetOldEmailsCache().SetChanged(true);
		}


		if (m_session.HasNetworkError()
				|| (m_visitedEmailHeadersCount >= SAVE_EMAIL_BATCH_SIZE && (int)m_persistEmails->size() > 0)) {
			// persist emails
			m_state = State_PersistEmails;
		} else {
			CheckDownloadState(msgTimestamp);
			if (m_state != State_PersistEmails && m_state != State_TrimEmails) {
				m_state = State_GetNextMessageToDownloadHeader;
			}
		}
	} catch (const std::exception e) {
		// removes email from persist email list
		PopEmail::PopEmailPtr popEmail = m_persistEmails->back();
		MojLogInfo(m_log, "Error in downloading email header for '%s': %s", popEmail->GetServerUID().c_str(), e.what());
		m_persistEmails->pop_back();

		if (m_session.HasNetworkError()
				|| (m_visitedEmailHeadersCount >= SAVE_EMAIL_BATCH_SIZE && (int)m_persistEmails->size() > 0)) {
			// persist emails
			m_state = State_PersistEmails;
		} else {
			m_state = State_GetNextMessageToDownloadHeader;
		}
	}

	CheckState();
	return MojErrNone;
}

bool SyncEmailsCommand::ShouldIncludeInFolder(const MojInt64& timestamp) {
	if (m_totalMessageCount >= PopConfig::MAX_EMAIL_COUNT_ON_DEVICE) {
		// when the number of emails in the inbox exceeds the max email count limit,
		// only emails that is new than the latest email in last sync can be added
		// to the inbox.
		//
		// Note: we can't update the value of 'm_latestEmailTimestamp' since the
		// order of newer emails are not guaranteed to be in chronological order.
		// we don't want to skip some of the newer emails that are newer than
		// last latest email but older than the current latest email.
		return timestamp > m_latestEmailTimestamp;
	} else {
		// if the max email acount limit is not reached, we will include any emails
		// that are within the sync window
		return timestamp >= m_cutOffTime;
	}
}

MojErr SyncEmailsCommand::SaveEmails(PopEmail::PopEmailPtrVectorPtr emails)
{
	m_saveEmailsResponseSlot.cancel();
	m_commandResult.reset(new PopCommandResult(m_saveEmailsResponseSlot));

	m_insertEmailsCommand.reset(new InsertEmailsCommand(m_session, emails));
	m_insertEmailsCommand->SetResult(m_commandResult);
	m_insertEmailsCommand->Run();

	return MojErrNone;
}

MojErr SyncEmailsCommand::SaveEmailsResponse()
{
	m_visitedEmailHeadersCount = 0;
	m_persistEmails->clear();

	if (m_session.HasNetworkError()) {
		// in case of network failure, skip delete server emails step.
		m_state = State_SaveUidCache;
	} else if (m_reconcileEmails.empty()) {
		m_state = State_TrimEmails;
	} else {
		m_state = State_GetNextMessageToDownloadHeader;
	}

	CheckState();

	return MojErrNone;
}

void SyncEmailsCommand::TrimEmails()
{
	m_trimEmailsResponseSlot.cancel();
	m_commandResult.reset(new PopCommandResult(m_trimEmailsResponseSlot));

	m_trimEmailsCommand.reset(new TrimFolderEmailsCommand(m_session, m_folderId, (m_totalMessageCount - PopConfig::MAX_EMAIL_COUNT_ON_DEVICE), m_uidCache));
	m_trimEmailsCommand->SetResult(m_commandResult);
	m_trimEmailsCommand->Run();
}

MojErr SyncEmailsCommand::TrimEmailsResponse()
{
	m_state = State_DeleteServerEmails;
	CheckState();

	return MojErrNone;
}

void SyncEmailsCommand::DeleteServerEmails()
{
	m_commandResult.reset(new PopCommandResult(m_deleteServerEmailsResponseSlot));

	m_delServerEmailCommand.reset(new DeleteServerEmailsCommand(m_session, m_localDeletedEmailUids, m_uidCache));
	m_delServerEmailCommand->SetResult(m_commandResult);
	m_delServerEmailCommand->Run();
}

MojErr SyncEmailsCommand::DeleteServerEmailsResponse()
{
	m_state = State_LoadLatestUidCache;
	CheckState();

	return MojErrNone;
}

void SyncEmailsCommand::LoadLatestUidCache()
{
	// Codes to handle Ninja writes
	m_latestUidCache = UidCache();
	m_loadUidCacheResponseSlot.cancel();

	m_commandResult.reset(new PopCommandResult(m_loadUidCacheResponseSlot));
	m_loadUidCacheCommand.reset(new LoadUidCacheCommand(m_session, m_account->GetAccountId(), m_latestUidCache));
	m_loadUidCacheCommand->SetResult(m_commandResult);
	m_loadUidCacheCommand->Run();
}

MojErr SyncEmailsCommand::LoadUidCacheResponse()
{
	try {
		m_commandResult->CheckException();

		if (m_latestUidCache.GetRev() == m_uidCache.GetRev()) {
			// UID cache hasn't been changed in database since last inbox sync or
			// latest changes have been handled.  Then update the UID cache with
			// the latest changes.
			m_state = State_SaveUidCache;
		} else {
			// keep track of the UIDs that has been deleted from server and
			// clear the previously deleted email UIDs
			m_alreadyDeletedUids.insert(m_localDeletedEmailUids.begin(), m_localDeletedEmailUids.end());
			m_localDeletedEmailUids.clear();
			// update local UID cache's revision to the latest
			m_uidCache.SetRev(m_latestUidCache.GetRev());

			// find the list of new pending deleted UIDs that has not been deleted
			// from server
			DeletedEmailsCache::CacheSet deleted = m_latestUidCache.GetDeletedEmailsCache().GetPendingDeletedCache();
			DeletedEmailsCache::CacheSet::iterator setItr;
			for (setItr = deleted.begin(); setItr != deleted.end(); setItr++) {
				std::string uid = *setItr;
				ReconcileEmailsCommand::LocalDeletedEmailInfo deletedInfo(MojObject::Undefined, uid);

				if (m_alreadyDeletedUids.find(deletedInfo) == m_alreadyDeletedUids.end()) {
					MojLogInfo(m_log, "A new pending deleted email UID '%s' is found", uid.c_str());
					m_localDeletedEmailUids.push_back(deletedInfo);
				} 
			}

			if (m_localDeletedEmailUids.empty()) {
				m_state = State_SaveUidCache;
			} else {
				m_state = State_DeleteServerEmails;
			}
		}

		CheckState();
	} catch(const std::exception& e) {
		Failure(e);
	} catch(...) {
		MailException exc("Unable to save emails' UID cache", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}

void SyncEmailsCommand::SaveUidCache()
{
	m_saveUidCacheResponseSlot.cancel();
	m_commandResult.reset(new PopCommandResult(m_saveUidCacheResponseSlot));
	m_saveUidCacheCommand.reset(new UpdateUidCacheCommand(m_session, m_uidCache));
	m_saveUidCacheCommand->SetResult(m_commandResult);
	m_saveUidCacheCommand->Run();
}

MojErr SyncEmailsCommand::SaveUidCacheResponse()
{
	try {
		m_commandResult->CheckException();
		m_state = State_Complete;
		CheckState();
	} catch(const std::exception& e) {
		Failure(e);
	} catch(...) {
		MailException exc("Unable to save emails' UID cache", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}

void SyncEmailsCommand::CheckState()
{
	if (!m_requests.empty() && m_state != State_HandleRequest) {
		m_orgState = m_state;
		m_state = State_HandleRequest;
	}

	MojLogDebug(m_log, "Sync state: %d", m_state);

	try {
		switch (m_state) {
		case State_ReconcileEmails:
			ReconcileEmails();
			break;
		case State_DeleteLocalEmails:
			DeleteLocalEmails();
			break;
		case State_GetNextMessageToDownloadHeader:
			GetNextMessageToDownloadHeader();
			break;
		case State_DownloadEmailHeader:
			// fetch email header
			FetchEmailHeader(m_reconcileInfo);
			break;
		case State_PersistEmails:
			// persist emails
			SaveEmails(m_persistEmails);
			break;
		case State_TrimEmails:
			// trim emails
			TrimEmails();
			break;
		case State_DeleteServerEmails:
			DeleteServerEmails();
			break;
		case State_LoadLatestUidCache:
			LoadLatestUidCache();
			break;
		case State_SaveUidCache:
			SaveUidCache();
			break;
		case State_HandleRequest:
			HandleNextRequest();
			break;
		case State_Complete:
		case State_Cancel:
			CommandComplete();
			break;
		case State_None:
			break;
		}
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown error in syncing emails", __FILE__, __LINE__));
	}
}

void SyncEmailsCommand::CommandComplete()
{
	MojLogInfo(m_log, "SyncEmailsCommand::CommandComplete");

	m_session.AutoDownloadEmails(m_folderId);
	m_session.SyncCompleted();
	m_session.GetSyncSession()->CommandCompleted(this);

	Complete();
}

void SyncEmailsCommand::Failure(const std::exception& ex)
{
	MojLogError(m_log, "SyncEmailsCommand::Failure");

	m_session.GetSyncSession()->CommandFailed(this, ex);
	PopSessionPowerCommand::Failure(ex);
}
