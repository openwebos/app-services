// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef ACTIVITYBUILDERFACTORY_H_
#define ACTIVITYBUILDERFACTORY_H_

#include "activity/Activity.h"
#include "activity/ActivityBuilder.h"
#include "core/MojRefCount.h"

class ActivityBuilderFactory : public MojRefCounted
{
public:
	static const char* const POP_ACTIVITY_CREATOR_ID;

	ActivityBuilderFactory();
	~ActivityBuilderFactory();

	void SetAccountId(const MojObject& accountId);

	void GetAccountPrefsWatchActivityName(MojString& name);
	void GetSentEmailsWatchActivityName(MojString& name);
	void GetScheduledSyncActivityName(MojString& name);
	void GetFolderRetrySyncActivityName(MojString& name, const MojObject& folderId);
	void GetMoveEmailsActivityName(MojString& name);
	void GetDeleteEmailsActivityName(MojString& name);

	/**
	 * Build account preference watch activity.
	 */
	void BuildAccountPrefsWatch(ActivityBuilder& builder, MojObject& accountRev);

	/**
	 * Build sent email watch activity.  This activity will invoke the POP service
	 * to move sent emails from outbox to sent folder.
	 */
	void BuildSentEmailsWatch(ActivityBuilder& builder, const MojObject& outboxFolderId, const MojObject& sentFolderId);

	/**
	 * Build schedule sync activity.
	 */
	void BuildScheduledSync(ActivityBuilder& builder, int intervalMins);

	/**
	 * Build folder retry sync activity.
	 */
	void BuildFolderRetrySync(ActivityBuilder& builder, const MojObject& folderId, int currInterval);

	void BuildMoveEmailsWatch(ActivityBuilder& builder);

	void BuildDeleteEmailsWatch(ActivityBuilder& builder);
private:
	static const char* const ACCOUNT_PREFS_WATCH_ACTIVITY_NAME_TEMPLATE;
	static const char* const ACCOUNT_PREFS_WATCH_DESC;
	static const char* const ACCOUNT_PREFS_WATCH_CALLBACK;

	static const char* const SENT_EMAILS_WATCH_ACTIVITY_NAME_TEMPLATE;
	static const char* const SENT_EMAILS_WATCH_DESC;
	static const char* const SENT_EMAILS_WATCH_CALLBACK;

	static const char* const SCHEDULED_SYNC_ACTIVITY_NAME_TEMPLATE;
	static const char* const SCHEDULED_SYNC_DESC;
	static const char* const SCHEDULED_SYNC_CALLBACK;

	static const char* const FOLDER_RETRY_SYNC_ACTIVITY_NAME_TEMPLATE;
	static const char* const FOLDER_RETRY_SYNC_DESC;
	static const char* const FOLDER_RETRY_SYNC_CALLBACK;

	static const char* const MOVE_EMAILS_ACTIVITY_NAME_TEMPLATE;
	static const char* const MOVE_EMAILS_DESC;
	static const char* const MOVE_EMAILS_CALLBACK;

	static const char* const DELETE_EMAILS_ACTIVITY_NAME_TEMPLATE;
	static const char* const DELETE_EMAILS_DESC;
	static const char* const DELETE_EMAILS_CALLBACK;

	// Retry intervals that will be used if an retry-able error occurs in folder sync
	static const int RETRY_INTERVAL_MINS[];

	int	 GetNextRetryIntervalMins(int currInterval);
	int  MinsToSecs(int mins)	{ return mins * 60; }

	void GetAccountPrefsWatchActivityDesc(MojString& desc);
	void GetSentEmailsWatchActivityDesc(MojString& desc);
	void GetScheduledSyncActivityDesc(MojString& desc);
	void GetFolderRetrySyncActivityDesc(MojString& desc, const MojObject& folderId);
	void GetMoveEmailsActivityDesc(MojString& desc);
	void GetDeleteEmailsActivityDesc(MojString& desc);

	MojObject		m_accountId;
};

typedef MojRefCountedPtr<ActivityBuilderFactory>	BuilderFactoryPtr;

#endif /* ACTIVITYBUILDERFACTORY_H_ */
