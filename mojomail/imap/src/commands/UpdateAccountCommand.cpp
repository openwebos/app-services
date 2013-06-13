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

#include "commands/UpdateAccountCommand.h"
#include "data/ImapAccountAdapter.h"
#include "activity/ActivityBuilder.h"
#include "ImapClient.h"
#include "data/DatabaseAdapter.h"
#include "activity/ImapActivityFactory.h"
#include "ImapPrivate.h"
#include "commands/UpdateFolderActivitiesCommand.h"
#include "activity/ActivitySet.h"

using namespace std;

UpdateAccountCommand::UpdateAccountCommand(ImapClient& client, ActivityPtr activity, bool credentialsChanged)
: ImapClientCommand(client),
  m_activitySet(new ActivitySet(client)),
  m_credentialsChanged(credentialsChanged),
  m_getAccountTransportSlot(this, &UpdateAccountCommand::GetAccountTransportResponse),
  m_updateFolderActivitiesSlot(this, &UpdateAccountCommand::UpdateFolderActivitiesDone),
  m_endActivitiesSlot(this, &UpdateAccountCommand::ActivitiesEnded),
  m_notifySmtpSlot(this, &UpdateAccountCommand::NotifySmtpResponse)
{
	// FIXME wait for adoption to complete
	// EndActivities will wait for us, but it's not as polite
	if(activity.get() && activity->CanAdopt()) {
		m_activitySet->AddActivity(activity);

		activity->Adopt(client);
		activity->SetEndOrder(Activity::EndOrder_Last);
	}
}

UpdateAccountCommand::~UpdateAccountCommand()
{
}

void UpdateAccountCommand::RunImpl()
{
	CommandTraceFunction();

	GetAccountTransport();
}

void UpdateAccountCommand::GetAccountTransport()
{
	CommandTraceFunction();

	m_accountId = m_client.GetAccountId();
	// Refresh the account for the client from the DBWatch
	m_imapAccountFinder.reset((new ImapAccountFinder(m_client, m_accountId, false)));
	m_imapAccountFinder->Run(m_getAccountTransportSlot);
}

MojErr UpdateAccountCommand::GetAccountTransportResponse()
{
	CommandTraceFunction();

	try {
		m_imapAccountFinder->GetResult()->CheckException();

		UpdateFolderActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void UpdateAccountCommand::UpdateFolderActivities()
{
	CommandTraceFunction();

	MojObject::ObjectVec emptyList; // update all folders

	m_updateFolderActivitiesCommand.reset(new UpdateFolderActivitiesCommand(m_client, emptyList));
	m_updateFolderActivitiesCommand->Run(m_updateFolderActivitiesSlot);
}

MojErr UpdateAccountCommand::UpdateFolderActivitiesDone()
{
	CommandTraceFunction();

	try {
		// ignore failure for update folder activities command

		// Update preference watch
		UpdatePreferencesWatchActivity(m_client.GetAccount().GetRevision());
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void UpdateAccountCommand::UpdatePreferencesWatchActivity(MojInt64 rev)
{
	CommandTraceFunction();

	ImapActivityFactory factory;
	ActivityBuilder ab;

	MojLogInfo(m_log, "account %s preferences watch updated; rev=%lld", AsJsonString(m_accountId).c_str(), rev);

	if(rev > 0) {
		factory.BuildPreferencesWatch(ab, m_accountId, rev);

		m_activitySet->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
	} else {
		MojLogCritical(m_log, "unknown preferences rev (couldn't load account?); can't create watch");

		m_activitySet->SetEndAction(Activity::EndAction_Complete);
	}

	m_activitySet->EndActivities(m_endActivitiesSlot);
}

MojErr UpdateAccountCommand::ActivitiesEnded()
{
	CommandTraceFunction();

	// After the account is updated, queue a sync
	SyncParams syncParams;
	syncParams.SetForce(true);
	syncParams.SetForceReconnect(true); // in case the server address changed
	syncParams.SetReason("updated account preferences");

	m_client.SyncAccount(syncParams);

	if(m_credentialsChanged) {
		NotifySmtp();
	} else {
		Done();
	}

	return MojErrNone;
}

//  Tell SMTP that the account has been updated; only if the credentials changed
void UpdateAccountCommand::NotifySmtp()
{
	CommandTraceFunction();

	MojErr err;
	MojObject payload;

	err = payload.put("accountId", m_client.GetAccountId());
	ErrorToException(err);

	m_client.SendRequest(m_notifySmtpSlot, "com.palm.smtp", "credentialsChanged", payload);
}

MojErr UpdateAccountCommand::NotifySmtpResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);
	} catch(const std::exception& e) {
		// log and ignore error
		MojLogError(m_log, "error calling SMTP credentialsChanged: %s", e.what());
	}

	Done();

	return MojErrNone;
}

void UpdateAccountCommand::Done()
{
	Complete();
}

void UpdateAccountCommand::Status(MojObject& status) const
{
	MojErr err;

	ImapClientCommand::Status(status);

	if(m_updateFolderActivitiesCommand.get()) {
		MojObject updateStatus;
		m_updateFolderActivitiesCommand->Status(updateStatus);
		err = status.put("m_updateFolderActivitiesCommand", updateStatus);
		ErrorToException(err);
	}

	if(m_activitySet.get()) {
		MojObject activityStatus;
		m_activitySet->Status(activityStatus);
		err = status.put("activitySet", activityStatus);
		ErrorToException(err);
	}
}
