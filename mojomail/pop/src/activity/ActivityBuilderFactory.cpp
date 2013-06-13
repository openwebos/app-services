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

#include "activity/ActivityBuilderFactory.h"
#include "data/EmailSchema.h"
#include "data/PopAccountAdapter.h"
#include "data/PopEmailAdapter.h"
#include "data/PopFolderAdapter.h"
#include "PopDefs.h"

const char* const ActivityBuilderFactory::POP_ACTIVITY_CREATOR_ID = "serviceId:com.palm.pop";

const char* const ActivityBuilderFactory::ACCOUNT_PREFS_WATCH_ACTIVITY_NAME_TEMPLATE = "POP accountId=%s/Prefs Watch";
const char* const ActivityBuilderFactory::ACCOUNT_PREFS_WATCH_DESC = "POP Activity on Account Preferences Change on accountId %s";
const char* const ActivityBuilderFactory::ACCOUNT_PREFS_WATCH_CALLBACK = "palm://com.palm.pop/accountUpdated";

const char* const ActivityBuilderFactory::SENT_EMAILS_WATCH_ACTIVITY_NAME_TEMPLATE = "POP accountId=%s/Sent Emails Watch";
const char* const ActivityBuilderFactory::SENT_EMAILS_WATCH_DESC = "POP Activity to move emails from Outbox to Sent folder on accountId %s";
const char* const ActivityBuilderFactory::SENT_EMAILS_WATCH_CALLBACK = "palm://com.palm.pop/moveToSentFolder";

const char* const ActivityBuilderFactory::SCHEDULED_SYNC_ACTIVITY_NAME_TEMPLATE = "POP accountId=%s/Scheduled Sync";
const char* const ActivityBuilderFactory::SCHEDULED_SYNC_DESC = "POP Activity for a Schedule Sync on the accountId %s";
const char* const ActivityBuilderFactory::SCHEDULED_SYNC_CALLBACK = "palm://com.palm.pop/syncAccount";

const char* const ActivityBuilderFactory::FOLDER_RETRY_SYNC_ACTIVITY_NAME_TEMPLATE = "POP accountId=%s/Retry Sync folderId=%s";
const char* const ActivityBuilderFactory::FOLDER_RETRY_SYNC_DESC = "POP Activity for Retrying to Sync a Folder %s in the Case of a Retry-able Error Occurred in previous Sync";
const char* const ActivityBuilderFactory::FOLDER_RETRY_SYNC_CALLBACK = "palm://com.palm.pop/syncFolder";

const char* const ActivityBuilderFactory::MOVE_EMAILS_ACTIVITY_NAME_TEMPLATE = "POP accountId=%s/Move Emails";
const char* const ActivityBuilderFactory::MOVE_EMAILS_DESC = "POP Activity for Moving Emails on accountId %s";
const char* const ActivityBuilderFactory::MOVE_EMAILS_CALLBACK = "palm://com.palm.pop/moveEmail";

const char* const ActivityBuilderFactory::DELETE_EMAILS_ACTIVITY_NAME_TEMPLATE = "POP accountId=%s/Delete Emails";
const char* const ActivityBuilderFactory::DELETE_EMAILS_DESC = "POP Activity for Deleting Emails on accountId %s";
const char* const ActivityBuilderFactory::DELETE_EMAILS_CALLBACK = "palm://com.palm.pop/deleteEmail";

// Retry intervals that will be used if an retry-able error occurs in folder sync
const int ActivityBuilderFactory::RETRY_INTERVAL_MINS[] = {1, 2, 5, 10, 30, 60};

ActivityBuilderFactory::ActivityBuilderFactory()
{

}

ActivityBuilderFactory::~ActivityBuilderFactory()
{

}

void ActivityBuilderFactory::SetAccountId(const MojObject& accountId)
{
	m_accountId = accountId;
}

void ActivityBuilderFactory::GetAccountPrefsWatchActivityName(MojString& name)
{
	MojErr err = name.format(ACCOUNT_PREFS_WATCH_ACTIVITY_NAME_TEMPLATE, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetAccountPrefsWatchActivityDesc(MojString& desc)
{
	MojErr err = desc.format(ACCOUNT_PREFS_WATCH_DESC, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetSentEmailsWatchActivityName(MojString& name)
{
	MojErr err = name.format(SENT_EMAILS_WATCH_ACTIVITY_NAME_TEMPLATE, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetSentEmailsWatchActivityDesc(MojString& desc)
{
	MojErr err = desc.format(SENT_EMAILS_WATCH_DESC, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetScheduledSyncActivityName(MojString& name)
{
	MojErr err = name.format(SCHEDULED_SYNC_ACTIVITY_NAME_TEMPLATE, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetScheduledSyncActivityDesc(MojString& desc)
{
	MojErr err = desc.format(SCHEDULED_SYNC_DESC, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetFolderRetrySyncActivityName(MojString& name, const MojObject& folderId)
{
	MojErr err = name.format(FOLDER_RETRY_SYNC_ACTIVITY_NAME_TEMPLATE, AsJsonString(m_accountId).c_str(), AsJsonString(folderId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetFolderRetrySyncActivityDesc(MojString& desc, const MojObject& folderId)
{
	MojErr err = desc.format(FOLDER_RETRY_SYNC_DESC, AsJsonString(folderId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetMoveEmailsActivityName(MojString& name)
{
	MojErr err = name.format(MOVE_EMAILS_ACTIVITY_NAME_TEMPLATE, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetMoveEmailsActivityDesc(MojString& desc)
{
	MojErr err = desc.format(MOVE_EMAILS_DESC, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetDeleteEmailsActivityName(MojString& name)
{
	MojErr err = name.format(DELETE_EMAILS_ACTIVITY_NAME_TEMPLATE, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::GetDeleteEmailsActivityDesc(MojString& desc)
{
	MojErr err = desc.format(DELETE_EMAILS_DESC, AsJsonString(m_accountId).c_str());
	ErrorToException(err);
}

void ActivityBuilderFactory::BuildAccountPrefsWatch(ActivityBuilder& builder, MojObject& accountRev)
{
	MojString name;
	GetAccountPrefsWatchActivityName(name);

	MojString desc;
	GetAccountPrefsWatchActivityDesc(desc);

	builder.SetName(name.data());
	builder.SetDescription(desc.data());
	builder.SetPersist(true);
	builder.SetExplicit(true);
	builder.SetImmediate(true, "low");

	MojDbQuery trigger;
	MojErr err = trigger.from(PopAccountAdapter::POP_ACCOUNT_KIND);
	ErrorToException(err);
	err = trigger.where(PopAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, m_accountId);
	ErrorToException(err);
	err = trigger.where("_revPop", MojDbQuery::OpGreaterThan, accountRev);
	ErrorToException(err);
	builder.SetDatabaseWatchTrigger(trigger);

	MojObject params;
	err = params.put(PopAccountAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	builder.SetCallback(ACCOUNT_PREFS_WATCH_CALLBACK, params);

	MojObject metadata;
	err = metadata.put(PopAccountAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	builder.SetMetadata(metadata);
}

void ActivityBuilderFactory::BuildSentEmailsWatch(ActivityBuilder& builder, const MojObject& outboxFolderId, const MojObject& sentFolderId)
{
	// activity to setup watch
	MojString name;
	GetSentEmailsWatchActivityName(name);

	// description of watch
	MojString desc;
	GetSentEmailsWatchActivityDesc(desc);

	builder.SetName(name.data());
	builder.SetDescription(desc.data());
	builder.SetPersist(true);
	builder.SetExplicit(true);
	builder.SetForeground(true);

	// setup trigger
	MojDbQuery trigger;
	MojErr err = trigger.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);
	err = trigger.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, outboxFolderId);
	ErrorToException(err);
	err = trigger.where("sendStatus.sent", MojDbQuery::OpEq, true);
	ErrorToException(err);
	builder.SetDatabaseWatchTrigger(trigger);

	// setup parameters (to be sent when trigger is called)
	MojObject params;
	err = params.put(PopFolderAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	err = params.put(PopFolderAdapter::OUTBOX_FOLDER_ID, outboxFolderId);
	ErrorToException(err);
	err = params.put(PopFolderAdapter::SENT_FOLDER_ID, sentFolderId);
	ErrorToException(err);
	builder.SetCallback(SENT_EMAILS_WATCH_CALLBACK, params);

	// put accountId in metadata
	MojObject metadata;
	err = metadata.put("accountId", m_accountId);
	ErrorToException(err);
	builder.SetMetadata(metadata);
}

void ActivityBuilderFactory::BuildScheduledSync(ActivityBuilder &builder, int intervalMins)
{
	// activity to setup watch
	MojString name;
	GetScheduledSyncActivityName(name);

	// description of watch
	MojString desc;
	GetScheduledSyncActivityDesc(desc);

	// sync interval in seconds
	int intervalSecs = MinsToSecs(intervalMins);

	builder.SetName(name.data());
	builder.SetDescription(desc.data());
	builder.SetPersist(true);
	builder.SetExplicit(true);
	builder.SetRequiresInternet(false);
	builder.SetSyncInterval(0, intervalSecs);
	builder.SetImmediate(true, "low");

	// setup parameters (to be sent when trigger is called)
	MojObject params;
	MojErr err = params.put(PopAccountAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	builder.SetCallback(SCHEDULED_SYNC_CALLBACK, params);

	// put accountId in metadata
	MojObject metadata;
	err = metadata.put("accountId", m_accountId);
	ErrorToException(err);
	builder.SetMetadata(metadata);
}

void ActivityBuilderFactory::BuildFolderRetrySync(ActivityBuilder &builder, const MojObject& folderId, int currInterval)
{
	// activity to setup watch
	MojString name;
	GetFolderRetrySyncActivityName(name, folderId);

	// description of watch
	MojString desc;
	GetFolderRetrySyncActivityDesc(desc, folderId);

	// retry interval in seconds
	int nextRetryInterval = GetNextRetryIntervalMins(currInterval);
	int intervalSecs = MinsToSecs(nextRetryInterval);

	builder.SetName(name.data());
	builder.SetDescription(desc.data());
	builder.SetPersist(true);
	builder.SetExplicit(true);
	builder.SetRequiresInternet(false);
	builder.SetImmediate(true, "low");
	builder.SetSyncInterval(0, intervalSecs);

	// setup parameters (to be sent when trigger is called)
	MojObject params;
	MojErr err = params.put(PopAccountAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	err = params.put("folderId", folderId);
	ErrorToException(err);
	err = params.put("lastRetryInterval", nextRetryInterval);
	ErrorToException(err);
	builder.SetCallback(FOLDER_RETRY_SYNC_CALLBACK, params);

	// put accountId in metadata
	MojObject metadata;
	err = metadata.put("accountId", m_accountId);
	ErrorToException(err);
	builder.SetMetadata(metadata);
}

void ActivityBuilderFactory::BuildMoveEmailsWatch(ActivityBuilder& builder)
{
	MojString name;
	GetMoveEmailsActivityName(name);

	MojString desc;
	GetMoveEmailsActivityDesc(desc);

	builder.SetName(name.data());
	builder.SetDescription(desc.data());
	builder.SetPersist(true);
	builder.SetExplicit(true);
	builder.SetForeground(true);

	// setup trigger
	MojDbQuery trigger;
	MojErr err = trigger.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);
	err = trigger.where("destFolderId", MojDbQuery::OpNotEq, MojObject());
	ErrorToException(err);
	builder.SetDatabaseWatchTrigger(trigger);

	// setup parameters (to be sent when trigger is called)
	MojObject params;
	err = params.put(PopFolderAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	builder.SetCallback(MOVE_EMAILS_CALLBACK, params);

	// put accountId in metadata
	MojObject metadata;
	err = metadata.put(PopFolderAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	builder.SetMetadata(metadata);
}

void ActivityBuilderFactory::BuildDeleteEmailsWatch(ActivityBuilder& builder)
{
	MojString name;
	GetDeleteEmailsActivityName(name);

	MojString desc;
	GetDeleteEmailsActivityDesc(desc);

	builder.SetName(name.data());
	builder.SetDescription(desc.data());
	builder.SetPersist(true);
	builder.SetExplicit(true);
	builder.SetImmediate(true, "low");

	// setup trigger
	MojDbQuery trigger;
	MojErr err = trigger.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);
	err = trigger.where("_del", MojDbQuery::OpEq, true);
	ErrorToException(err);
	builder.SetDatabaseWatchTrigger(trigger);

	// setup parameters (to be sent when trigger is called)
	MojObject params;
	err = params.put(PopFolderAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	builder.SetCallback(DELETE_EMAILS_CALLBACK, params);

	// put accountId in metadata
	MojObject metadata;
	err = metadata.put(PopFolderAdapter::ACCOUNT_ID, m_accountId);
	ErrorToException(err);
	builder.SetMetadata(metadata);
}

int	ActivityBuilderFactory::GetNextRetryIntervalMins(int currInterval)
{
	// TODO: right now it will always retry in 5 minutes or later when internet connection is available
	//       But we should get the previous retry interval to determine the next one
	return 5;
}
