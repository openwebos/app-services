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

#include "commands/ScheduleRetryCommand.h"
#include "activity/ActivityBuilder.h"
#include "activity/ImapActivityFactory.h"
#include "commands/DeleteActivitiesCommand.h"
#include "data/EmailAccountAdapter.h"
#include "ImapClient.h"
#include "ImapPrivate.h"

int ScheduleRetryCommand::INITIAL_RETRY_SECONDS = 60;		// first retry after 1 minute
int ScheduleRetryCommand::SECOND_RETRY_SECONDS = 5 * 60;	// second retry after 5 minutes
float ScheduleRetryCommand::RETRY_MULTIPLIER = 1.5;			// 1.5x backoff thereafter
int ScheduleRetryCommand::MAX_RETRY_SECONDS = 30 * 60;

ScheduleRetryCommand::ScheduleRetryCommand(ImapClient& client, const MojObject& folderId, SyncParams syncParams, const std::string& reason)
: ImapClientCommand(client),
  m_folderId(folderId),
  m_syncParams(syncParams),
  m_scheduleRetrySlot(this, &ScheduleRetryCommand::ScheduleRetryResponse),
  m_deleteActivitiesSlot(this, &ScheduleRetryCommand::CancelActivitiesDone),
  m_updateAccountSlot(this, &ScheduleRetryCommand::UpdateAccountResponse)
{
}

ScheduleRetryCommand::~ScheduleRetryCommand()
{
}

void ScheduleRetryCommand::RunImpl()
{
	ScheduleRetry();
}

void ScheduleRetryCommand::ScheduleRetry()
{
	CommandTraceFunction();

	ImapActivityFactory factory;
	ActivityBuilder ab;

	// Get current retry interval from account
	EmailAccount::RetryStatus retryStatus = m_client.GetAccount().GetRetry();

	int interval = retryStatus.GetInterval();

	if(interval < INITIAL_RETRY_SECONDS) {
		interval = INITIAL_RETRY_SECONDS;
	} else if(interval < SECOND_RETRY_SECONDS) {
		interval = SECOND_RETRY_SECONDS;
	} else if(interval >= MAX_RETRY_SECONDS) {
		interval = MAX_RETRY_SECONDS;
	} else {
		// TODO: only update this on actual retry?
		interval *= RETRY_MULTIPLIER;
	}

	// Update account just in case it wasn't within the limit
	retryStatus.SetInterval(interval);
	retryStatus.SetCount(retryStatus.GetCount() + 1);
	m_client.GetAccount().SetRetry(retryStatus);

	factory.BuildSyncRetry(ab, m_client.GetAccountId(), m_folderId, interval, m_reason);

	MojErr err;
	MojObject payload;
	err = payload.put("activity", ab.GetActivityObject());
	ErrorToException(err);
	err = payload.put("start", true);
	ErrorToException(err);
	err = payload.put("replace", true);

	MojLogInfo(m_log, "scheduling retry in %.1f minutes for account %s", float(interval) / 60, m_client.GetAccountIdString().c_str());

	m_client.SendRequest(m_scheduleRetrySlot, "com.palm.activitymanager", "create", payload);
}

MojErr ScheduleRetryCommand::ScheduleRetryResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		CancelActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void ScheduleRetryCommand::CancelActivities()
{
	CommandTraceFunction();

	// Delete anything that includes the folderId in the name
	MojString folderIdSubstring;
	MojErr err = folderIdSubstring.format("folderId=%s", AsJsonString(m_folderId).c_str());
	ErrorToException(err);

	// Don't delete the retry activity
	ImapActivityFactory factory;
	MojString retryActivityName = factory.GetSyncRetryName(m_client.GetAccountId(), m_folderId);

	m_deleteActivitiesCommand.reset(new DeleteActivitiesCommand(m_client));
	m_deleteActivitiesCommand->SetIncludeNameFilter(folderIdSubstring);
	m_deleteActivitiesCommand->SetExcludeNameFilter(retryActivityName);
	m_deleteActivitiesCommand->Run(m_deleteActivitiesSlot);
}

MojErr ScheduleRetryCommand::CancelActivitiesDone()
{
	CommandTraceFunction();

	try {
		m_deleteActivitiesCommand->GetResult()->CheckException();

		UpdateAccount();
	} CATCH_AS_FAILURE

	return MojErrNone;
}
void ScheduleRetryCommand::UpdateAccount()
{
	CommandTraceFunction();

	MojObject accountObj;

	EmailAccountAdapter::SerializeAccountRetryStatus(m_client.GetAccount(), accountObj);

	m_client.GetDatabaseInterface().UpdateAccountRetry(m_updateAccountSlot, m_client.GetAccountId(), accountObj);
}

MojErr ScheduleRetryCommand::UpdateAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

