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

#ifndef IMAPACTIVITYFACTORY_H_
#define IMAPACTIVITYFACTORY_H_

#include "ImapCoreDefs.h"

class ActivityBuilder;

class ImapActivityFactory
{
public:
	ImapActivityFactory();
	virtual ~ImapActivityFactory();

	static const char* const PREFS_WATCH_NAME;
	static const char* const PREFS_WATCH_CALLBACK;

	static const char* const FOLDER_WATCH_NAME;
	static const char* const FOLDER_WATCH_CALLBACK;

	static const char* const OUTBOX_WATCH_NAME;
	static const char* const OUTBOX_WATCH_CALLBACK;

	static const char* const DRAFTS_WATCH_NAME;
	static const char* const DRAFTS_WATCH_CALLBACK;

	static const char* const MAINTAIN_IDLE_NAME;
	static const char* const MAINTAIN_IDLE_CALLBACK;

	static const char* const IDLE_WAKEUP_NAME;
	static const char* const IDLE_WAKEUP_CALLBACK;

	static const char* const SCHEDULED_SYNC_NAME;
	static const char* const SCHEDULED_SYNC_CALLBACK;

	static const char* const SYNC_RETRY_NAME;
	static const char* const SYNC_RETRY_CALLBACK;

	static const char* const CONNECT_NAME;

	MojString GetPreferencesWatchName(const MojObject& accountId);
	void BuildPreferencesWatch(ActivityBuilder& ab, const MojObject& accountId, MojInt64 rev);

	MojString GetFolderWatchName(const MojObject& accountId, const MojObject& folderId);
	void BuildFolderWatch(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, MojInt64 rev);

	MojString GetOutboxWatchName(const MojObject& accountId, const MojObject& folderId);
	void BuildOutboxWatch(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, MojInt64 rev);

	MojString GetDraftsWatchName(const MojObject& accountId, const MojObject& folderId);
	void BuildDraftsWatch(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, MojInt64 rev);

	MojString GetStartIdleName(const MojObject& accountId, const MojObject& folderId);
	void BuildStartIdle(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId);

	MojString GetIdleWakeupName(const MojObject& accountId, const MojObject& folderId);
	void BuildIdleWakeup(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, int seconds);

	MojString GetScheduledSyncName(const MojObject& accountId, const MojObject& folderId);
	void BuildScheduledSync(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, int seconds, bool requireFair);

	MojString GetSyncRetryName(const MojObject& accountId, const MojObject& folderId);
	void BuildSyncRetry(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, int seconds, const std::string& reason = "");

	void BuildConnect(ActivityBuilder& ab, const MojObject& accountId, MojUInt64 uniqueId);

	static MojString FormatName(const char* name, const MojObject& accountId);
	static MojString FormatName(const char* name, const MojObject& accountId, const MojObject& folderId);

	static void SetMetadata(MojObject& metadata, const char* name, const MojObject& accountId, const MojObject& folderId);
	static void SetMetadata(MojObject& metadata, const char* name, const MojObject& accountId);

	static void SetNetworkRequirements(ActivityBuilder& ab, bool requireFair);
};

#endif /* IMAPACTIVITYFACTORY_H_ */
