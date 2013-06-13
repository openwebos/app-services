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

#include "commands/SyncFolderCommand.h"
#include "ImapClient.h"
#include "activity/ActivityBuilder.h"
#include "activity/ImapActivityFactory.h"
#include "activity/NetworkStatus.h"
#include "activity/ActivitySet.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "ImapPrivate.h"

SyncFolderCommand::SyncFolderCommand(ImapClient& client, const MojObject& folderId, SyncParams params)
: ImapClientCommand(client),
  m_folderId(folderId),
  m_syncParams(params),
  m_activitySet(new ActivitySet(client)),
  m_getFolderSlot(this, &SyncFolderCommand::GetFolderResponse),
  m_smtpSlot(this, &SyncFolderCommand::SmtpResponse),
  m_startActivitiesSlot(this, &SyncFolderCommand::StartActivitiesDone),
  m_adoptOnlySlot(this, &SyncFolderCommand::AdoptDone),
  m_cleanupActivitiesSlot(this, &SyncFolderCommand::CleanupActivitiesDone)
{
}

SyncFolderCommand::~SyncFolderCommand()
{
}

void SyncFolderCommand::RunImpl()
{
	GetFolder();
}

void SyncFolderCommand::GetFolder()
{
	m_client.GetDatabaseInterface().GetById(m_getFolderSlot, m_folderId);
}

MojErr SyncFolderCommand::GetFolderResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojObject folder;

		DatabaseAdapter::GetOneResult(response, folder);

		PrepareSync();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncFolderCommand::PrepareSync()
{
	if(m_folderId == m_client.GetAccount().GetOutboxFolderId()) {
		SyncOutbox();
	} else {
		bool force = m_syncParams.GetForce();
		bool disallowAutoSync = false;
		string disallowAutoSyncReason;

		if(m_client.GetAccount().IsManualSync()) {
			disallowAutoSync = true;
			disallowAutoSyncReason = "account is set to manual sync";
		}

		const MailError::ErrorInfo& accountError = m_client.GetAccount().GetError();
		if(accountError.errorCode == MailError::BAD_USERNAME_OR_PASSWORD) {
			disallowAutoSync = true;
			disallowAutoSyncReason = "bad username or password";
		}

		if(!disallowAutoSync && !m_client.GetAccount().HasPassword()) {
			disallowAutoSync = true;
			disallowAutoSyncReason = "no credentials available";

			// Set account error so the UI is informed about this
			MailError::ErrorInfo errorInfo(MailError::MISSING_CREDENTIALS);
			m_client.UpdateAccountStatus(errorInfo);

			// Don't allow force sync either
			force = false;
		}

		if(disallowAutoSync && !force) {
			// If we're set to manual sync, don't actually sync unless it's forced
			MojLogInfo(m_log, "not syncing; %s", disallowAutoSyncReason.c_str());

			m_activitySet->AddActivities(m_syncParams.GetSyncActivities());
			m_activitySet->AddActivities(m_syncParams.GetConnectionActivities());

			// Adopt the activities, and then complete them to get rid of them.
			m_activitySet->SetEndAction(Activity::EndAction_Complete);
			m_activitySet->StartActivities(m_adoptOnlySlot);
		} else {
			StartActivities();
		}
	}
}

void SyncFolderCommand::SyncOutbox()
{
	CommandTraceFunction();

	// Sync sent emails to server
	m_client.CheckOutbox(m_syncParams);

	// Notify SMTP service
	MojObject payload = m_syncParams.GetPayload();
	
	if(payload.empty()) {
		MojErr err;
		err = payload.put("accountId", m_client.GetAccountId());
		ErrorToException(err);

		err = payload.put("folderId", m_client.GetAccount().GetOutboxFolderId());
		ErrorToException(err);

		err = payload.put("force", m_syncParams.GetForce());
		ErrorToException(err);
	}

	m_client.SendRequest(m_smtpSlot, "com.palm.smtp", "syncOutbox", payload);
}

MojErr SyncFolderCommand::SmtpResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);
	} catch(const exception& e) {
		MojLogWarning(m_log, "error syncing outbox: %s", e.what());
	}

	Complete();

	return MojErrNone;
}

void SyncFolderCommand::StartActivities()
{
	CommandTraceFunction();

	m_activitySet->AddActivities(m_syncParams.GetAllActivities());
	m_activitySet->StartActivities(m_startActivitiesSlot);
}

MojErr SyncFolderCommand::StartActivitiesDone()
{
	CommandTraceFunction();

	try {
		// Remove sync activity from our activity set since the folder sync session
		m_activitySet->ClearActivities();

		m_client.StartFolderSync(m_folderId, m_syncParams);

		CleanupActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

MojErr SyncFolderCommand::AdoptDone()
{
	try {
		CleanupActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncFolderCommand::CleanupActivities()
{
	CommandTraceFunction();

	m_activitySet->EndActivities(m_cleanupActivitiesSlot);
}

MojErr SyncFolderCommand::CleanupActivitiesDone()
{
	CommandTraceFunction();

	Complete();
	return MojErrNone;
}

void SyncFolderCommand::Cleanup()
{
	// Clean up just to be safe
	m_startActivitiesSlot.cancel();
	m_cleanupActivitiesSlot.cancel();

	ImapClientCommand::Cleanup();
}

void SyncFolderCommand::Status(MojObject& status) const
{
	MojErr err;
	ImapClientCommand::Status(status);

	MojObject activitySetStatus;
	m_activitySet->Status(activitySetStatus);

	err = status.put("activitySet", activitySetStatus);
	ErrorToException(err);
}

std::string SyncFolderCommand::Describe() const
{
	if(!m_syncParams.GetReason().empty())
		return "SyncFolderCommand [folderId: " + AsJsonString(m_folderId) + ", reason: " + m_syncParams.GetReason() + "]";
	else
		return ImapClientCommand::Describe();
}
