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

#include "commands/ScheduleRetryCommand.h"
#include "activity/ActivityBuilder.h"
#include "commands/DeleteActivitiesCommand.h"
#include "data/EmailAccountAdapter.h"
#include "PopClient.h"
#include "PopDefs.h"

int ScheduleRetryCommand::INITIAL_RETRY_SECONDS = 60;
int ScheduleRetryCommand::MAX_RETRY_SECONDS = 30 * 60;
float ScheduleRetryCommand::RETRY_MULTIPLIER = 1.5;

ScheduleRetryCommand::ScheduleRetryCommand(PopClient& client, const MojObject& folderId, EmailAccount::AccountError err)
: PopClientCommand(client, "Schedule Retry"),
  m_folderId(folderId),
  m_accntErr(err),
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
	try {
		if (!m_client.GetAccount().get()) {
			MojString err;
			err.format("Account is not loaded for '%s'", AsJsonString(
					m_client.GetAccountId()).c_str());
			throw MailException(err.data(), __FILE__, __LINE__);
		}

		ScheduleRetry();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception in scheduling POP account retry", __FILE__, __LINE__));
	}
}

void ScheduleRetryCommand::ScheduleRetry()
{
	ActivityBuilder ab;

	// Get current retry interval from account
	EmailAccount::RetryStatus retryStatus = m_client.GetAccount()->GetRetry();

	int interval = retryStatus.GetInterval();

	if(interval <= INITIAL_RETRY_SECONDS) {
		interval = INITIAL_RETRY_SECONDS;
	} else if(interval >= MAX_RETRY_SECONDS) {
		interval = MAX_RETRY_SECONDS;
	} else {
		// TODO: only update this on actual retry?
		interval *= RETRY_MULTIPLIER;
	}

	// Update account just in case it wasn't within the limit
	retryStatus.SetInterval(interval);
	m_client.GetAccount()->SetRetry(retryStatus);

	m_client.GetActivityBuilderFactory()->BuildFolderRetrySync(ab, m_folderId, interval);

	MojErr err;
	MojObject payload;
	err = payload.put("activity", ab.GetActivityObject());
	ErrorToException(err);
	err = payload.put("start", true);
	ErrorToException(err);
	err = payload.put("replace", true);

	MojLogInfo(m_log, "Creating retry activity in activity manager: %s", AsJsonString(payload).c_str());
	m_client.SendRequest(m_scheduleRetrySlot, "com.palm.activitymanager", "create", payload);
}

MojErr ScheduleRetryCommand::ScheduleRetryResponse(MojObject& response, MojErr err)
{
	try {
		MojLogInfo(m_log, "Retry activity creation: %s", AsJsonString(response).c_str());
		ErrorToException(err);

		CancelActivities();
	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in schedule retry response: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in schedule retry response");
		Failure(MailException("Unknown exception in schedule retry response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void ScheduleRetryCommand::CancelActivities()
{
	// Delete anything that includes the folderId in the name
	MojString folderIdSubstring;
	MojErr err = folderIdSubstring.format("folderId=%s", AsJsonString(m_folderId).c_str());
	ErrorToException(err);

	MojString retryActivityName;
	m_client.GetActivityBuilderFactory()->GetFolderRetrySyncActivityName(retryActivityName, m_folderId);

	m_deleteActivitiesCommand.reset(new DeleteActivitiesCommand(m_client));
	m_deleteActivitiesCommand->SetIncludeNameFilter(folderIdSubstring);
	m_deleteActivitiesCommand->SetExcludeNameFilter(retryActivityName);
	m_deleteActivitiesCommand->Run(m_deleteActivitiesSlot);
}

MojErr ScheduleRetryCommand::CancelActivitiesDone()
{
	try {
		m_deleteActivitiesCommand->GetResult()->CheckException();

		UpdateAccount();
	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in cancel activities done: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in cancel activities done");
		Failure(MailException("Unknown exception in cancel activities done", __FILE__, __LINE__));
	}

	return MojErrNone;
}
void ScheduleRetryCommand::UpdateAccount()
{
	MojObject accountObj;

	EmailAccountAdapter::SerializeAccountRetryStatus(*m_client.GetAccount().get(), accountObj);

	m_client.GetDatabaseInterface().UpdateAccountRetry(m_updateAccountSlot, m_client.GetAccountId(), accountObj);
}

MojErr ScheduleRetryCommand::UpdateAccountResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		Complete();
	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in update account response: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in update account response");
		Failure(MailException("Unknown exception in update account response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

