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

#include "activity/ImapActivityFactory.h"
#include "activity/ActivityBuilder.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailSchema.h"
#include "data/ImapAccountAdapter.h"
#include "data/ImapEmailAdapter.h"
#include "ImapPrivate.h"
#include "ImapConfig.h"

// Be careful when changing any of these constants, otherwise old activities
// may not get overwritten properly.

const char* const ImapActivityFactory::PREFS_WATCH_NAME			= "IMAP preferences watch";
const char* const ImapActivityFactory::PREFS_WATCH_CALLBACK		= "palm://com.palm.imap/accountUpdated";

const char* const ImapActivityFactory::FOLDER_WATCH_NAME		= "IMAP folder watch";
const char* const ImapActivityFactory::FOLDER_WATCH_CALLBACK	= "palm://com.palm.imap/syncUpdate";

const char* const ImapActivityFactory::OUTBOX_WATCH_NAME		= "IMAP outbox watch";
const char* const ImapActivityFactory::OUTBOX_WATCH_CALLBACK	= "palm://com.palm.imap/sentEmails";

const char* const ImapActivityFactory::DRAFTS_WATCH_NAME		= "IMAP draft email watch";
const char* const ImapActivityFactory::DRAFTS_WATCH_CALLBACK	= "palm://com.palm.imap/draftSaved";

const char* const ImapActivityFactory::MAINTAIN_IDLE_NAME		= "IMAP idle setup";
const char* const ImapActivityFactory::MAINTAIN_IDLE_CALLBACK	= "palm://com.palm.imap/startIdle";

const char* const ImapActivityFactory::IDLE_WAKEUP_NAME			= "IMAP idle wakeup";
const char* const ImapActivityFactory::IDLE_WAKEUP_CALLBACK		= "palm://com.palm.imap/wakeupIdle";

const char* const ImapActivityFactory::SCHEDULED_SYNC_NAME		= "IMAP scheduled sync";
const char* const ImapActivityFactory::SCHEDULED_SYNC_CALLBACK	= "palm://com.palm.imap/syncAccount";

const char* const ImapActivityFactory::SYNC_RETRY_NAME			= "IMAP sync retry";
const char* const ImapActivityFactory::SYNC_RETRY_CALLBACK		= "palm://com.palm.imap/syncFolder";

const char* const ImapActivityFactory::CONNECT_NAME				= "IMAP network check";

ImapActivityFactory::ImapActivityFactory()
{
}

ImapActivityFactory::~ImapActivityFactory()
{
}

MojString ImapActivityFactory::FormatName(const char* name, const MojObject& accountId)
{
	MojString formattedName;
	MojErr err = formattedName.format("%s/accountId=%s",
			name, AsJsonString(accountId).c_str());
	ErrorToException(err);
	return formattedName;
}

MojString ImapActivityFactory::FormatName(const char* name, const MojObject& accountId, const MojObject& folderId)
{
	MojString formattedName;
	MojErr err = formattedName.format("%s/accountId=%s/folderId=%s",
			name, AsJsonString(accountId).c_str(), AsJsonString(folderId).c_str());
	ErrorToException(err);
	return formattedName;
}

void ImapActivityFactory::SetMetadata(MojObject& metadata, const char* name, const MojObject& accountId, const MojObject& folderId)
{
	MojErr err;

	err = metadata.putString("name", name);
	ErrorToException(err);

	err = metadata.put("accountId", accountId);
	ErrorToException(err);

	err = metadata.put("folderId", folderId);
	ErrorToException(err);
}

void ImapActivityFactory::SetMetadata(MojObject& metadata, const char* name, const MojObject& accountId)
{
	MojErr err;

	err = metadata.putString("name", name);
	ErrorToException(err);

	err = metadata.put("accountId", accountId);
	ErrorToException(err);
}

void ImapActivityFactory::SetNetworkRequirements(ActivityBuilder& ab, bool requireFair)
{
	ab.SetRequiresInternet(false);

	if(requireFair && !ImapConfig::GetConfig().GetIgnoreNetworkStatus()) {
		ab.SetRequiresInternetConfidence("fair");
	}
}

MojString ImapActivityFactory::GetPreferencesWatchName(const MojObject& accountId)
{
	return FormatName(PREFS_WATCH_NAME, accountId);
}

void ImapActivityFactory::BuildPreferencesWatch(ActivityBuilder& ab, const MojObject& accountId, MojInt64 rev)
{
	MojErr err;

	ab.SetName( GetPreferencesWatchName(accountId) );
	ab.SetDescription("IMAP account preferences watch");
	ab.SetPersist(true);
	ab.SetForeground(true);

	// Metadata
	MojObject metadata;
	SetMetadata(metadata, PREFS_WATCH_NAME, accountId);
	ab.SetMetadata(metadata);

	MojDbQuery trigger;
	err = trigger.from(ImapAccountAdapter::SCHEMA);
	ErrorToException(err);
	err = trigger.where(ImapAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);
	err = trigger.where(ImapAccountAdapter::CONFIG_REV, MojDbQuery::OpGreaterThan, rev);
	ErrorToException(err);
	ab.SetDatabaseWatchTrigger(trigger);

	MojObject params;
	err = params.put("accountId", accountId);
	ErrorToException(err);

	ab.SetCallback(PREFS_WATCH_CALLBACK, params);
}

MojString ImapActivityFactory::GetFolderWatchName(const MojObject& accountId, const MojObject& folderId)
{
	return FormatName(FOLDER_WATCH_NAME, accountId, folderId);
}

void ImapActivityFactory::BuildFolderWatch(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, MojInt64 rev)
{
	MojErr err;
	MojDbQuery query;

	ab.SetName( GetFolderWatchName(accountId, folderId) );
	ab.SetDescription("Watches for updates to emails");
	ab.SetExplicit(true);
	ab.SetPersist(true);
	SetNetworkRequirements(ab, true);
	ab.SetImmediate(true, "low");

	// Metadata
	MojObject metadata;
	SetMetadata(metadata, FOLDER_WATCH_NAME, accountId, folderId);
	ab.SetMetadata(metadata);

	// Query
	err = query.from(ImapEmailAdapter::IMAP_EMAIL_KIND);
	ErrorToException(err);
	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);
	err = query.where(ImapEmailAdapter::UPSYNC_REV, MojDbQuery::OpGreaterThan, rev);
	ErrorToException(err);
	err = query.includeDeleted(true);
	ErrorToException(err);

	ab.SetDatabaseWatchTrigger(query);

	// Callback
	MojObject callbackParams;
	err = callbackParams.put("accountId", accountId);
	ErrorToException(err);
	err = callbackParams.put("folderId", folderId);
	ErrorToException(err);

	ab.SetCallback(FOLDER_WATCH_CALLBACK, callbackParams);
}

MojString ImapActivityFactory::GetOutboxWatchName(const MojObject& accountId, const MojObject& folderId)
{
	return FormatName(OUTBOX_WATCH_NAME, accountId, folderId);
}

void ImapActivityFactory::BuildOutboxWatch(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, MojInt64 rev)
{
	MojErr err;

	ab.SetName( GetOutboxWatchName(accountId, folderId) );
	ab.SetDescription("Watches for sent emails in outbox");
	ab.SetPersist(true);
	ab.SetExplicit(true);
	ab.SetForeground(true);

	// Metadata
	MojObject metadata;
	SetMetadata(metadata, OUTBOX_WATCH_NAME, accountId, folderId);
	ab.SetMetadata(metadata);

	MojDbQuery query;

	err = query.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);
	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	// sendStatus.sent
	MojString sentProp;
	sentProp.format("%s.%s",EmailSchema::SEND_STATUS, EmailSchema::SendStatus::SENT);
	err = query.where(sentProp.data(), MojDbQuery::OpEq, true);
	ErrorToException(err);

	ab.SetDatabaseWatchTrigger(query);

	// Callback
	MojObject callbackParams;
	err = callbackParams.put("accountId", accountId);
	ErrorToException(err);

	ab.SetCallback(OUTBOX_WATCH_CALLBACK, callbackParams);
}

MojString ImapActivityFactory::GetDraftsWatchName(const MojObject& accountId, const MojObject& folderId)
{
	return FormatName(DRAFTS_WATCH_NAME, accountId, folderId);
}

void ImapActivityFactory::BuildDraftsWatch(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, MojInt64 rev)
{
	MojErr err;

	ab.SetName( GetDraftsWatchName(accountId, folderId) );
	ab.SetDescription("Watches for updates to draft emails");
	ab.SetPersist(true);
	ab.SetExplicit(true);
	ab.SetForeground(true);

	// Metadata
	MojObject metadata;
	SetMetadata(metadata, DRAFTS_WATCH_NAME, accountId, folderId);
	ab.SetMetadata(metadata);

	MojDbQuery query;

	err = query.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);
	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	MojString editedDraftFlag;
	editedDraftFlag.format("%s.%s", EmailSchema::FLAGS, EmailSchema::Flags::EDITEDDRAFT);
	err = query.where(editedDraftFlag.data(), MojDbQuery::OpEq, true);
	ErrorToException(err);

	ab.SetDatabaseWatchTrigger(query);

	// Callback
	MojObject callbackParams;
	err = callbackParams.put("accountId", accountId);
	ErrorToException(err);
	err = callbackParams.put("folderId", accountId);
	ErrorToException(err);

	ab.SetCallback(DRAFTS_WATCH_CALLBACK, callbackParams);
}

MojString ImapActivityFactory::GetStartIdleName(const MojObject& accountId, const MojObject& folderId)
{
	return FormatName(MAINTAIN_IDLE_NAME, accountId, folderId);
}

void ImapActivityFactory::BuildStartIdle(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId)
{
	MojErr err;

	ab.SetName( GetStartIdleName(accountId, folderId) );
	ab.SetDescription("Starts a push connection when the network is available");
	ab.SetExplicit(false);
	ab.SetPersist(true);
	ab.SetImmediate(true, "low");
	SetNetworkRequirements(ab, false);

	// Metadata
	MojObject metadata;
	SetMetadata(metadata, MAINTAIN_IDLE_NAME, accountId, folderId);
	ab.SetMetadata(metadata);

	// Callback
	MojObject params;
	err = params.put("accountId", accountId);
	ErrorToException(err);
	err = params.put("folderId", folderId);
	ErrorToException(err);

	ab.SetCallback(MAINTAIN_IDLE_CALLBACK, params);
}

MojString ImapActivityFactory::GetIdleWakeupName(const MojObject& accountId, const MojObject& folderId)
{
	return FormatName(IDLE_WAKEUP_NAME, accountId, folderId);
}

void ImapActivityFactory::BuildIdleWakeup(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, int seconds)
{
	ab.SetName( GetIdleWakeupName(accountId, folderId) );
	ab.SetDescription("Wakes up idle connection");
	ab.SetExplicit(false); // goes away after a cancel
	ab.SetPersist(false);
	ab.SetForeground(true); // must run immediately
	ab.SetPowerOptions(false, true);

	// Wakeup
	ab.SetStartDelay(seconds);
}

MojString ImapActivityFactory::GetScheduledSyncName(const MojObject& accountId, const MojObject& folderId)
{
	return FormatName(SCHEDULED_SYNC_NAME, accountId, folderId);
}

void ImapActivityFactory::BuildScheduledSync(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, int seconds, bool requireFair)
{
	MojErr err;

	ab.SetName( GetScheduledSyncName(accountId, folderId) );

	MojString desc;
	err = desc.format("Scheduled sync every %d minutes", seconds/60);
	ErrorToException(err);

	ab.SetDescription(desc.data());
	ab.SetExplicit(true);
	ab.SetPersist(true);
	ab.SetImmediate(true, "low");
	SetNetworkRequirements(ab, requireFair);

	// Metadata
	MojObject metadata;
	SetMetadata(metadata, SCHEDULED_SYNC_NAME, accountId, folderId);
	ab.SetMetadata(metadata);

	// Wakeup
	ab.SetSyncInterval(0, seconds);

	// Callback
	MojObject params;
	err = params.put("accountId", accountId);
	ErrorToException(err);

	ab.SetCallback(SCHEDULED_SYNC_CALLBACK, params);
}

MojString ImapActivityFactory::GetSyncRetryName(const MojObject& accountId, const MojObject& folderId)
{
	return FormatName(SYNC_RETRY_NAME, accountId, folderId);
}

void ImapActivityFactory::BuildSyncRetry(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId, int seconds, const std::string& reason)
{
	MojErr err;

	ab.SetName( GetSyncRetryName(accountId, folderId) );

	MojString desc;
	err = desc.format("Retry sync after %d seconds", seconds);
	ErrorToException(err);

	ab.SetDescription(desc.data());
	ab.SetExplicit(true);
	ab.SetPersist(true);
	ab.SetImmediate(true, "low");
	SetNetworkRequirements(ab, seconds <= 5 * 60);

	// Metadata
	MojObject metadata;
	SetMetadata(metadata, SYNC_RETRY_NAME, accountId, folderId);
	ab.SetMetadata(metadata);

	// Wakeup
	ab.SetSyncInterval(seconds, 0);

	// Callback
	MojObject params;
	err = params.put("accountId", accountId);
	ErrorToException(err);
	err = params.put("folderId", folderId);
	ErrorToException(err);

	MojObject retry;
	if(!reason.empty()) {
		err = retry.putString("reason", reason.c_str());
		ErrorToException(err);
	}

	err = retry.put("interval", seconds);
	ErrorToException(err);

	err = params.put("retry", retry);
	ErrorToException(err);

	ab.SetCallback(SYNC_RETRY_CALLBACK, params);
}

// This is used if we don't have any other activity to get network status from
void ImapActivityFactory::BuildConnect(ActivityBuilder& ab, const MojObject& accountId, MojUInt64 uniqueId)
{
	MojString name;
	MojErr err = name.format("%s - %lld", CONNECT_NAME, uniqueId);
	ErrorToException(err);

	ab.SetName( FormatName(name.data(), accountId) );
	ab.SetDescription("Activity for connecting to server");
	ab.SetExplicit(false);
	ab.SetPersist(false);
	ab.SetForeground(true);
	ab.SetRequiresInternet(false);
}
