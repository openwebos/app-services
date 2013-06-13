// @@@LICENSE
//
//      Copyright (c) 2010-2013 LG Electronics, Inc.
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


#ifndef SYNCSESSION_H_
#define SYNCSESSION_H_

#include "client/BaseSyncSession.h"
#include "client/SyncParams.h"

class ImapClient;
class ScheduleRetryCommand;

/**
 * Implements our transport-specific code associated with a sync session.
 * Includes logic for querying the list of changed objects and updating
 * the lastSyncRev and activities after the sync session is done.
 */
class SyncSession : public BaseSyncSession
{
public:
	SyncSession(ImapClient& client, const MojObject& folderId);
	virtual ~SyncSession();

	virtual void SetQueueStopped();

protected:
	// Implement abstract methods
	virtual void SyncSessionReady();
	virtual void SyncSessionComplete();

	virtual void GetNewChanges(MojDbClient::Signal::SlotRef slot, MojObject folderId, MojInt64 rev, MojDbQuery::Page &page);
	virtual void GetById(MojDbClient::Signal::SlotRef slot, const MojObject& id);
	virtual void Merge(MojDbClient::Signal::SlotRef slot, const MojObject& obj);

	// Overrides BaseSyncSession
	virtual void UpdateActivities();

	// Overrides BaseSyncSession
	virtual void UpdateAndEndActivities();

	virtual MojErr ScheduleRetryDone();

	void ClearRetry();
	virtual MojErr ClearRetryResponse(MojObject& response, MojErr err);

	// Update activities
	virtual void UpdateFolderWatch();
	virtual void UpdateScheduledSyncActivity();
	virtual void UpdateRetryActivity();
	virtual void UpdateIdleActivity();
	virtual void UpdateMiscActivities();

	// Sync state
	virtual bool IsAccountSync();
	virtual void BuildFolderStatus(MojObject& folderStatus);

	virtual MojString GetCapabilityProvider();
	virtual MojString GetBusAddress();

	ImapClient&		m_client;

	bool			m_needsRetry;

	MojRefCountedPtr<ScheduleRetryCommand> m_scheduleRetryCommand;

	MojSignal<>::Slot<SyncSession>	m_retrySlot;
	MojDbClient::Signal::Slot<SyncSession>	m_clearRetrySlot;
};

#endif /* SYNCSESSION_H_ */
