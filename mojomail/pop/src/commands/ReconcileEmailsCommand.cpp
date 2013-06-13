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

#include "boost/shared_ptr.hpp"
#include "commands/ReconcileEmailsCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailSchema.h"
#include "data/UidCacheAdapter.h"
#include "data/PopEmailAdapter.h"
#include "PopDefs.h"

const int ReconcileEmailsCommand::LOAD_EMAIL_BATCH_SIZE 	= 100;

ReconcileEmailsCommand::ReconcileEmailsCommand(PopSession& session,
		const MojObject& folderId,
		const MojInt64& cutoffTime,
		MojInt64& latestEmailTimestamp,
		int& messageCount,
		ReconcileInfoQueue& queue,
		UidCache& uidCache,
		MojObject::ObjectVec& oldEmailIds,
		MojObject::ObjectVec& serverDeletedEmailIds,
		LocalDeletedEmailsVec& localDeletedEmailUids)
: PopSessionCommand(session),
  m_account(session.GetAccount()),
  m_folderId(folderId),
  m_cutOffTime(cutoffTime),
  m_latestEmailTimestamp(latestEmailTimestamp),
  m_messageCount(messageCount),
  m_uidMap(session.GetUidMap()),
  m_uidCache(uidCache),
  m_reconcileQueue(queue),
  m_oldEmailIds(oldEmailIds),
  m_serverDeletedEmailIds(serverDeletedEmailIds),
  m_localDeletedEmailUids(localDeletedEmailUids),
  m_getLocalEmailsResponseSlot(this, &ReconcileEmailsCommand::GetLocalEmailsResponse),
  m_getUidCacheResponseSlot(this, &ReconcileEmailsCommand::GetUidCacheResponse),
  m_getDeletedEmailsResponseSlot(this, &ReconcileEmailsCommand::GetDeletedEmailsResponse)

{
}

ReconcileEmailsCommand::~ReconcileEmailsCommand()
{
}

void ReconcileEmailsCommand::RunImpl()
{
	InitReconcileMap(m_uidMap);
	GetLocalEmails();
}

void ReconcileEmailsCommand::InitReconcileMap(boost::shared_ptr<UidMap> uidMap)
{
	UidMap::UidInfoMap uidInfoMap = uidMap->GetUidInfoMap();
	UidMap::UidInfoMap::iterator itr;

	for (itr = uidInfoMap.begin(); itr != uidInfoMap.end(); itr++) {
		UidMap::MessageInfoPtr msgInfo = itr->second;
		ReconcileInfoPtr infoPtr(new ReconcileInfo());
		infoPtr->SetMessageInfo(*msgInfo);

		std::string uid = msgInfo->GetUid();
		int msgNum = msgInfo->GetMessageNumber();
		infoPtr->SetStatus(Status_Fetch_Header);
		m_reconcileUidMap[uid] = infoPtr;
		m_reconcileMsgNumMap[msgNum] = infoPtr;
	}
}

void ReconcileEmailsCommand::GetLocalEmails()
{
	CommandTraceFunction();
	// TODO: need to get last sync rev from sync session
	MojInt32 lastRev = 0;
	m_getLocalEmailsResponseSlot.cancel();  // in case this is called multiple times
	m_session.GetDatabaseInterface().GetEmailSyncList(m_getLocalEmailsResponseSlot, m_folderId, lastRev, true, m_localEmailsPage, LOAD_EMAIL_BATCH_SIZE);
}

MojErr ReconcileEmailsCommand::GetLocalEmailsResponse(MojObject& response, MojErr err)
{
	try {
		MojLogDebug(m_log, "Got local emails response: %s", AsJsonString(response).c_str());

		// Check database response
		ErrorToException(err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		MojObject::ArrayIterator it;
		err = results.arrayBegin(it);
		ErrorToException(err);

		for (; it != results.arrayEnd(); ++it) {
			MojObject& emailObj = *it;
			MojErr err;

			MojObject id;
			err = emailObj.getRequired(PopEmailAdapter::ID, id);
			ErrorToException(err);

			MojString uid;
			err = emailObj.getRequired(PopEmailAdapter::SERVER_UID, uid);
			ErrorToException(err);
			std::string uidStr(uid);

			MojInt64 timestamp;
			err = emailObj.getRequired(EmailSchema::TIMESTAMP, timestamp);

			if (!m_uidMap->HasUid(uidStr)) {
				// email is not found in server
				if (!m_account->IsDeleteOnDevice()) {
					// if the user chooses not to delete emails on device that
					// have been deleted from the server, that means these emails
					// won't been deleted.  So we need to include these emails as wells.
					m_messageCount++;
				}
			
				// TODO: need to handle the case when an email is moved from other
				// folders into inbox.  May add a virtual flag in POP email to indicate
				// that the email has been locally move into inbox.
				m_serverDeletedEmailIds.push(id);
			} else {
				boost::shared_ptr<ReconcileInfo> info = m_reconcileUidMap[uidStr];
				if (timestamp < m_cutOffTime) {
					// email is beyond sync window

					// add id to the array of IDs that will be deleted soon
					m_oldEmailIds.push(id);
					// add this old email into old emails' cache
					m_uidCache.GetOldEmailsCache().AddToCache(std::string(uid), timestamp);
					m_uidCache.GetOldEmailsCache().SetChanged(true);

					// mark reconcile info as old email
					info->SetStatus(Status_Old_Email);
					info->SetTimestamp(timestamp);
				} else {
					// email is still within sync window
				
					// mark reconcile info as downloaded
					info->SetStatus(Status_Downloaded);
					info->SetTimestamp(timestamp);

					if (timestamp > m_latestEmailTimestamp) {
						m_latestEmailTimestamp = timestamp;
					}

					// keep track of how many emails will be remained in the inbox
					m_messageCount++;
				}
			}
		}

		bool hasMore = DatabaseAdapter::GetNextPage(response, m_localEmailsPage);
		if(hasMore) {
			// Get next batch of emails
			MojLogDebug(m_log, "getting another batch of local emails");
			GetLocalEmails();
		} else {
			MojLogDebug(m_log, "Got local deleted emails cache");
			GetLocalDeletedEmails();
		}
	} catch(const std::exception& e) {
		MojLogError(m_log, "Failed to get local emails: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to get local emails", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}

void ReconcileEmailsCommand::GetLocalDeletedEmails()
{
	CommandTraceFunction();
	// TODO: need to get last sync rev from sync session
	MojInt32 lastRev = 0;
	m_getDeletedEmailsResponseSlot.cancel();  // in case this function is called multiple times
	m_session.GetDatabaseInterface().GetDeletedEmails(m_getDeletedEmailsResponseSlot,
			m_folderId, lastRev, m_localEmailsPage, LOAD_EMAIL_BATCH_SIZE);
}

MojErr ReconcileEmailsCommand::GetDeletedEmailsResponse(MojObject& response, MojErr err)
{
	try {
		MojLogDebug(m_log, "Got local deleted emails response: %s", AsJsonString(response).c_str());

		// Check database response
		ErrorToException(err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		if(results.size() > 0) {
			MojLogInfo(m_log, "Got %d local deleted emails", results.size());
		}

		MojObject::ArrayIterator it;
		err = results.arrayBegin(it);
		ErrorToException(err);

		// Read email properties: _id, UID, flags
		// If you need additional properties, modify the database query
		for (; it != results.arrayEnd(); ++it) {
			MojObject& emailObj = *it;

			MojObject id;
			err = emailObj.getRequired(PopEmailAdapter::ID, id);
			ErrorToException(err);

			MojString uid;
			err = emailObj.getRequired(PopEmailAdapter::SERVER_UID, uid);
			ErrorToException(err);

			std::string uidStr(uid);
			m_localDeletedEmailUids.push_back(LocalDeletedEmailInfo(id, uidStr));

			ReconcileInfoPtr infoPtr = m_reconcileUidMap[uidStr];
			if (infoPtr.get()) {
				m_reconcileUidMap.erase(std::string(uid));
				UidMap::MessageInfo info = infoPtr->GetMessageInfo();
				int msgNum = info.GetMessageNumber();
				m_reconcileMsgNumMap.erase(msgNum);
			}
		}

		bool hasMore = DatabaseAdapter::GetNextPage(response, m_localEmailsPage);

		if(hasMore) {
			// Get next batch of emails
			MojLogDebug(m_log, "getting another batch of local deleted emails");
			GetLocalDeletedEmails();
		} else {
			MojLogDebug(m_log, "Got old emails cache");
			GetUidCache();
		}
	} catch(const std::exception& e) {
		MojLogError(m_log, "Failed to sync folder: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to get deleted emails", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}

void ReconcileEmailsCommand::GetUidCache()
{
	m_getUidCacheResponseSlot.cancel();
	m_commandResult.reset(new PopCommandResult(m_getUidCacheResponseSlot));
	m_loadCacheCommand.reset(new LoadUidCacheCommand(m_session, m_account->GetAccountId(), m_uidCache));
	m_loadCacheCommand->SetResult(m_commandResult);
	m_loadCacheCommand->Run();
}


MojErr ReconcileEmailsCommand::GetUidCacheResponse()
{
	try {
		m_commandResult->CheckException();

		// if the sync window has been changed, update the olde email cache.
		m_uidCache.GetOldEmailsCache().UpdateCacheWithSyncWindow(m_cutOffTime);

		// move to next action in this command
		MojLogInfo(m_log, "Got all local info");
		ReconcileEmails();
	} catch(const std::exception& e) {
		MojLogError(m_log, "Failed to sync folder: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to get old emails cache", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}

void ReconcileEmailsCommand::ReconcileEmails()
{
	try {
		// reconcile local deleted emails
		DeletedEmailsCache::CacheSet deleted = m_uidCache.GetDeletedEmailsCache().GetLocalDeletedCache();
		DeletedEmailsCache::CacheSet::iterator setItr;
		if (m_account->IsDeleteFromServer()) {
			// account has been altered so that this flag was changed from false to true
			for (setItr = deleted.begin(); setItr != deleted.end(); setItr++) {
				// since there are local emails pending to be deleted, delete
				// these emails from the server if these emails still exist in
				// the server.
				std::string uid = *setItr;
				ReconcileInfoPtr infoPtr = m_reconcileUidMap[uid];

				if (infoPtr.get()) {
					m_localDeletedEmailUids.push_back(LocalDeletedEmailInfo(
							MojObject::Undefined, uid));
					m_reconcileUidMap.erase(uid);
					UidMap::MessageInfo info = infoPtr->GetMessageInfo();
					int msgNum = info.GetMessageNumber();
					m_reconcileMsgNumMap.erase(msgNum);
				}
			}
			m_uidCache.GetDeletedEmailsCache().ClearLocalDeletedEmailCache();
		} else {
			for (setItr = deleted.begin(); setItr != deleted.end(); setItr++) {
				std::string uid = *setItr;
				ReconcileInfoPtr infoPtr = m_reconcileUidMap[uid];

				if (infoPtr.get()) {
					m_reconcileUidMap.erase(uid);
					UidMap::MessageInfo info = infoPtr->GetMessageInfo();
					int msgNum = info.GetMessageNumber();
					m_reconcileMsgNumMap.erase(msgNum);
				}
			}
		}

		// reconcile pending deleted emails
		deleted = m_uidCache.GetDeletedEmailsCache().GetPendingDeletedCache();
		for (setItr = deleted.begin(); setItr != deleted.end(); setItr++) {
			// since there are emails pending to be deleted, delete
			// these emails from the server if these emails still exist in
			// the server.
			std::string uid = *setItr;
			ReconcileInfoPtr infoPtr = m_reconcileUidMap[uid];

			if (infoPtr.get()) {
				m_localDeletedEmailUids.push_back(LocalDeletedEmailInfo(
						MojObject::Undefined, uid));
				m_reconcileUidMap.erase(uid);
				UidMap::MessageInfo info = infoPtr->GetMessageInfo();
				int msgNum = info.GetMessageNumber();
				m_reconcileMsgNumMap.erase(msgNum);
			}
		}
		m_uidCache.GetDeletedEmailsCache().ClearPendingDeletedEmailCache();

		// reconcile older email's cache
		OldEmailsCache::CacheMap cache = m_uidCache.GetOldEmailsCache().GetCacheMap();
		OldEmailsCache::CacheMap::iterator mapItr;
		std::vector<std::string> deletedCache;
		for (mapItr = cache.begin(); mapItr != cache.end(); mapItr++) {
			std::string uid = mapItr->first;
			MojInt64 ts = mapItr->second;

			ReconcileInfoUidMap::iterator infoItr = m_reconcileUidMap.find(uid);
			if (infoItr == m_reconcileUidMap.end()) {
				deletedCache.push_back(uid);
			} else {
				ReconcileInfoPtr info = infoItr->second;
				if (info.get()) {
					info->SetStatus(Status_Old_Email);
					info->SetTimestamp(ts);
				}
			}
		}
		std::vector<std::string>::iterator delItr;
		for (delItr = deletedCache.begin(); delItr != deletedCache.end(); delItr++) {
			cache.erase(*delItr);
		}

		std::priority_queue<UidMsgNumPair> uidMsgNumQueue;
		ReconcileInfoUidMap::iterator itr;
		for (itr = m_reconcileUidMap.begin(); itr != m_reconcileUidMap.end(); itr++) {
			std::string uid = itr->first;
			ReconcileInfoPtr infoPtr = itr->second;

			if (infoPtr.get()) {
				UidMap::MessageInfo info = infoPtr->GetMessageInfo();
				int msgNum = info.GetMessageNumber();

				UidMsgNumPair pair(uid, msgNum);
				uidMsgNumQueue.push(pair);
			}
		}

		while (!uidMsgNumQueue.empty()) {
			UidMsgNumPair pair = uidMsgNumQueue.top();
			int msgNum = pair.GetMessageNumber();
			ReconcileInfoPtr infoPtr = m_reconcileMsgNumMap[msgNum];

			m_reconcileQueue.push(infoPtr);
			uidMsgNumQueue.pop();
		}

		m_reconcileUidMap.clear();
		m_reconcileMsgNumMap.clear();
	} catch (const std::exception& e) {
		MojLogError(m_log, "Failed to reconcile emails in folder: %s", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to reconcile emails in folder", __FILE__, __LINE__);
		Failure(exc);
	}

	Complete();
}
