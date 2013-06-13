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

#ifndef SYNCSESSION_H_
#define SYNCSESSION_H_

#include "activity/ActivityBuilder.h"
#include "activity/ActivityBuilderFactory.h"
#include "client/BaseSyncSession.h"
#include "data/EmailAccount.h"
#include "PopClient.h"
#include <vector>

class PopClient;

class SyncSession : public BaseSyncSession
{
public:
	SyncSession(PopClient& client, const MojObject& folderId);
	virtual ~SyncSession();

	virtual void	CommandCompleted(Command* command);
	void 			AttachActivity(ActivityPtr activity);
	void 			WaitForSessionComplete(MojSignal<>::SlotRef completedSlot);
	void			SetAccountError(EmailAccount::AccountError accntErr);
private:
	static const int GET_CHANGES_BATCH_SIZE;

	virtual void GetNewChanges(MojDbClient::Signal::SlotRef slot, MojObject folderId, MojInt64 rev, MojDbQuery::Page &page);
	virtual void GetById(MojDbClient::Signal::SlotRef slot, const MojObject& id);
	virtual void Merge(MojDbClient::Signal::SlotRef slot, const MojObject& obj);

	virtual void SetWatchParams(ActivityBuilder& actBuilder);

	virtual void UpdateActivities();

	// Overrides BaseSyncSession
	virtual void UpdateAndEndActivities();
	virtual MojErr ScheduleRetryDone();
	virtual void UpdateScheduledSyncActivity();
	virtual void UpdateRetryActivity();
	virtual void SyncSessionComplete();
	virtual MojErr UpdateAccountResponse(MojObject& response, MojErr err);

	// Sync state
	virtual MojString GetCapabilityProvider();
	virtual MojString GetBusAddress();
	virtual void	  StartSpinner();
	// Is this an account sync (i.e. are we syncing the inbox?)
	virtual bool 	  IsAccountSync();

	PopClient& 					m_client;
	bool						m_needsRetry;
	BuilderFactoryPtr			m_builderFactory;
	EmailAccount::AccountError	m_accountError;
	MojSignal<>					m_completedSignal;
	MojDbClient::Signal::Slot<SyncSession>	m_updateAccountSlot;
};

typedef MojRefCountedPtr<SyncSession> SyncSessionPtr;

#endif /* SYNCSESSION_H_ */
