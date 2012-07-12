// @@@LICENSE
//
//      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

#include "client/SyncSession.h"
#include "ImapClient.h"
#include "activity/ActivityBuilder.h"
#include "data/DatabaseInterface.h"
#include "data/EmailSchema.h"
#include "data/ImapEmailAdapter.h"
#include "db/MojDbQuery.h"
#include "ImapPrivate.h"
#include "activity/ImapActivityFactory.h"
#include "commands/ScheduleRetryCommand.h"
#include "data/EmailAccountAdapter.h"
#include "data/DatabaseAdapter.h"
#include "data/SyncStateAdapter.h"
#include "data/FolderAdapter.h"

#define CATCH_AS_ERROR \
	catch(const exception& e) { HandleError(e, __FILE__, __LINE__); } \
	catch(...) { HandleError(boost::unknown_exception(), __FILE__, __LINE__); }

SyncSession::SyncSession(ImapClient& client, const MojObject& folderId)
: BaseSyncSession(client, client.GetLogger(), client.GetAccountId(), folderId),
  m_client(client),
  m_needsRetry(false),
  m_retrySlot(this, &SyncSession::ScheduleRetryDone),
  m_clearRetrySlot(this, &SyncSession::ClearRetryResponse)
{

}

SyncSession::~SyncSession()
{
}

void SyncSession::SyncSessionReady()
{
	BaseSyncSession::SyncSessionReady();
}

void SyncSession::SyncSessionComplete()
{
	BaseSyncSession::SyncSessionComplete();
	m_needsRetry = false;
}

void SyncSession::GetNewChanges(MojDbClient::Signal::SlotRef slot, MojObject folderId, MojInt64 rev, MojDbQuery::Page &page)
{
	m_client.GetDatabaseInterface().GetEmailChanges(slot, folderId, rev, page);
}

void SyncSession::GetById(MojDbClient::Signal::SlotRef slot, const MojObject& id)
{
	m_client.GetDatabaseInterface().GetById(slot, id);
}

void SyncSession::Merge(MojDbClient::Signal::SlotRef slot, const MojObject& obj)
{
	m_client.GetDatabaseInterface().UpdateFolder(slot, obj);
}

void SyncSession::SetQueueStopped()
{
	// Indicates that we've been disconnected and may need to schedule a retry
	if((IsStarting() || IsActive())
	&& (m_readyCommands.size() > 0 || m_waitingCommands.size() > 0 || m_failedCommands.size() > 0)) {
		MojLogInfo(m_log, "[sync session %s] needs to retry due to queue stopping", AsJsonString(m_folderId).c_str());
		m_needsRetry = true;
	}
}

void SyncSession::UpdateAndEndActivities()
{
	string reason; // unused for now

	if(IsAccountSync() || m_client.GetAccount().GetRetry().GetCount() < 1) {
		if(m_failedCommands.size() > 0) {
			MojLogInfo(m_log, "[sync session %s] needs retry due to failed commands", AsJsonString(m_folderId).c_str());
			m_needsRetry = true;
		}

	} else {
		// Non-inbox folders will only retry once
		m_needsRetry = false;
	}

	if(m_needsRetry) {
		SyncParams syncParams; // FIXME get appropriate sync params

		MojRefCountedPtr<ScheduleRetryCommand> command(new ScheduleRetryCommand(m_client, m_folderId, syncParams, ""));
		command->Run(m_retrySlot);
	} else {
		MojLogDebug(m_log, "[sync session %s] clearing retry", AsJsonString(m_folderId).c_str());
		ClearRetry();
	}
}

MojErr SyncSession::ScheduleRetryDone()
{
	BaseSyncSession::UpdateAndEndActivities();

	return MojErrNone;
}

// FIXME this should probably be handled somewhere else
void SyncSession::ClearRetry()
{
	MojObject accountObj;

	m_client.GetAccount().ClearRetry();

	EmailAccountAdapter::SerializeAccountRetryStatus(m_client.GetAccount(), accountObj);

	m_client.GetDatabaseInterface().UpdateAccountRetry(m_clearRetrySlot, m_client.GetAccountId(), accountObj);
}

MojErr SyncSession::ClearRetryResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		BaseSyncSession::UpdateAndEndActivities();
	} CATCH_AS_ERROR

	return MojErrNone;
}

MojString SyncSession::GetCapabilityProvider()
{
	MojString s;
	s.assign("com.palm.imap.mail");
	return s;
}

MojString SyncSession::GetBusAddress()
{
	MojString s;
	s.assign("com.palm.imap");
	return s;
}

bool SyncSession::IsAccountSync()
{
	return m_folderId == m_client.GetAccount().GetInboxFolderId();
}

void SyncSession::BuildFolderStatus(MojObject& folderStatus)
{
	MojErr err;

	string status;

	if(m_client.IsPushReady(m_folderId)) {
		status = FolderAdapter::SYNC_STATUS_PUSH;
	} else if(m_client.GetAccount().IsManualSync()) {
		status = FolderAdapter::SYNC_STATUS_MANUAL;
	} else {
		status = FolderAdapter::SYNC_STATUS_SCHEDULED;
	}

	err = folderStatus.putString(FolderAdapter::SYNC_STATUS, status.c_str());
	ErrorToException(err);
}

void SyncSession::UpdateActivities()
{
	if(m_client.DisableAccountInProgress()) {
		// Make sure we don't create any new activities
		m_activities->SetEndAction(Activity::EndAction_Cancel);
	} else if(!m_needsRetry) {
		m_activities->SetEndAction(Activity::EndAction_Complete);

		UpdateFolderWatch();
		UpdateScheduledSyncActivity();
		UpdateIdleActivity();
		UpdateMiscActivities();

		UpdateRetryActivity();
	}
}

void SyncSession::UpdateFolderWatch()
{
	// Update watch on folder
	ImapActivityFactory factory;
	ActivityBuilder ab;

	MojObject accountId = m_client.GetAccountId();

	if(!m_client.GetAccount().IsManualSync()) {
		// Update watch with new sync rev
		factory.BuildFolderWatch(ab, m_client.GetAccountId(), m_folderId, m_lastSyncRev);
		m_activities->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
	} else {
		// Remove watch
		ActivityPtr activity = m_activities->GetOrCreateActivity(factory.GetScheduledSyncName(accountId, m_folderId));
		activity->SetEndAction(Activity::EndAction_Cancel);
	}
}

void SyncSession::UpdateScheduledSyncActivity()
{
	//MojLogDebug(m_log, "updating scheduled sync activity on folder %s", AsJsonString(m_folderId).c_str());

	MojObject accountId = m_client.GetAccountId();

	ImapActivityFactory factory;
	ActivityBuilder ab;

	if(m_folderId == m_client.GetAccount().GetInboxFolderId() && !m_client.GetAccount().IsManualSync() && !m_client.GetAccount().IsPush()) {
		int intervalSeconds = m_client.GetAccount().GetSyncFrequencyMins() * 60;
		factory.BuildScheduledSync(ab, accountId, m_folderId, intervalSeconds, true);

		m_activities->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
	}
}

void SyncSession::UpdateRetryActivity()
{
	ImapActivityFactory factory;

	// Cancel retry activity on successful sync
	ActivityPtr activity = m_activities->GetOrCreateActivity(factory.GetSyncRetryName(m_client.GetAccountId(), m_folderId));
	activity->SetEndAction(Activity::EndAction_Cancel);

	// We want to remove the retry activity last, only after we're sure that the
	// scheduled sync and watch activities have been created.
	activity->SetEndOrder(Activity::EndOrder_Last);
}

void SyncSession::UpdateMiscActivities()
{
	ImapActivityFactory factory;

	if(m_folderId == m_client.GetAccount().GetDraftsFolderId()) {
		ActivityBuilder ab;
		factory.BuildDraftsWatch(ab, m_client.GetAccountId(), m_folderId, m_lastSyncRev);

		m_activities->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
	}
}

void SyncSession::UpdateIdleActivity()
{
#if 0
	ImapActivityFactory factory;
	ActivityBuilder ab;

	if(m_client.IsPushReady(m_folderId)) {
		factory.BuildStartIdle(ab, m_client.GetAccountId(), m_folderId);

		// Create WITHOUT replacing if it already exists
		ActivityPtr activity = Activity::PrepareNewActivity(ab, true, false);
		m_activities->AddActivity(activity);

		// Kick off create right now so that the activity set will manage it properly
		// FIXME should be handled within the activity set
		activity->Create(m_client);
	}
#endif
}
