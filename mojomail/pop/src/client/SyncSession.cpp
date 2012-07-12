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

#include "client/SyncSession.h"
#include "PopDefs.h"
#include "PopErrors.h"
#include "data/DatabaseAdapter.h"
#include "data/SyncStateAdapter.h"

const int SyncSession::GET_CHANGES_BATCH_SIZE = 50;

SyncSession::SyncSession(PopClient& client, const MojObject& folderId)
: BaseSyncSession(client, PopClient::s_log, client.GetAccount()->GetAccountId(), folderId),
  m_client(client),
  m_needsRetry(false),
  m_builderFactory(m_client.GetActivityBuilderFactory()),
  m_completedSignal(this),
  m_updateAccountSlot(this, &SyncSession::UpdateAccountResponse)
{
}

SyncSession::~SyncSession()
{
}

void SyncSession::AttachActivity(ActivityPtr activity)
{
	if (activity.get()) {
		if (IsActive()) {
			m_activities->AddActivity(activity);
		} else if (IsEnding()) {
			m_queuedActivities->AddActivity(activity);
		}
	}
}

void SyncSession::CommandCompleted(Command* command)
{
	// POP transport wants to explicitly stop the sync session.
	std::vector<CommandPtr>::iterator it;
	for(it = m_readyCommands.begin(); it != m_readyCommands.end(); ++it) {
		if(it->get() == command) {
			m_readyCommands.erase(it);
			break;
		}
	}
}

void SyncSession::WaitForSessionComplete(MojSignal<>::SlotRef completedSlot)
{
	m_completedSignal.connect(completedSlot);
}

void SyncSession::SetAccountError(EmailAccount::AccountError accntErr)
{
	m_accountError = accntErr;
}

void SyncSession::GetNewChanges(MojDbClient::Signal::SlotRef slot, MojObject folderId, MojInt64 rev, MojDbQuery::Page &page)
{
	m_client.GetDatabaseInterface().GetLocalEmailChanges(slot, folderId, rev, page, GET_CHANGES_BATCH_SIZE);
}

void SyncSession::GetById(MojDbClient::Signal::SlotRef slot, const MojObject& id)
{
	m_client.GetDatabaseInterface().GetById(slot, id);
}

void SyncSession::Merge(MojDbClient::Signal::SlotRef slot, const MojObject& obj)
{
	m_client.GetDatabaseInterface().UpdateItem(slot, obj);
}

void SyncSession::SetWatchParams(ActivityBuilder& actBuilder)
{
	// POP may not need to set up watch activity for folder sync
}

void SyncSession::UpdateAndEndActivities()
{
	if(!m_readyCommands.empty() || !m_waitingCommands.empty()) {
		m_needsRetry = true;
	}

	if(!m_failedCommands.empty()) {
		m_needsRetry = true;
	}

	try {
		BaseSyncSession::UpdateAndEndActivities();
	} catch (const std::exception& e) {
		HandleError(e, __FILE__, __LINE__);
	} catch (...) {
		HandleError(boost::unknown_exception(), __FILE__, __LINE__);
	}
}

MojErr SyncSession::ScheduleRetryDone()
{
	try {
		BaseSyncSession::UpdateAndEndActivities();
	} catch(const std::exception& e) {
		HandleError(e, __FILE__, __LINE__);
	} catch(...) {
		HandleError(boost::unknown_exception(), __FILE__, __LINE__);
	}

	return MojErrNone;
}

void SyncSession::UpdateActivities()
{
	MojLogInfo(m_log, "updating sync session %p activities", this);

	if (m_accountError.errorCode != MailError::NONE) {
		MojLogInfo(m_log, "--- account error: [%d]%s", m_accountError.errorCode, m_accountError.errorText.c_str());
	}

	if(PopErrors::IsRetryError(m_accountError)) {
		m_client.ScheduleRetrySync(m_accountError);
	} else {
		UpdateScheduledSyncActivity();
		UpdateRetryActivity();
	}
}

void SyncSession::UpdateScheduledSyncActivity()
{
	MojLogInfo(m_log, "updating scheduled sync activity on folder %s", AsJsonString(m_folderId).c_str());

	PopClient::AccountPtr account = m_client.GetAccount();
	MojObject accountId = account->GetAccountId();
	ActivityBuilder ab;

	if(!account->IsManualSync()) {
		int interval = account->GetSyncFrequencyMins();

		MojLogInfo(m_log, "setting sync interval to %d minutes", interval);

		m_builderFactory->BuildScheduledSync(ab, interval);

		MojObject actObj = ab.GetActivityObject();;
		MojLogInfo(m_log, "Replacing scheduled activity: %s", AsJsonString(actObj).c_str());

		m_activities->ReplaceActivity(ab.GetName(), actObj);
	} else {
		MojString scheduledActName;
		m_builderFactory->GetScheduledSyncActivityName(scheduledActName);

		// Remove activity
		ActivityPtr activity = m_activities->GetOrCreateActivity(scheduledActName);
		activity->SetEndAction(Activity::EndAction_Cancel);
	}
}

void SyncSession::UpdateRetryActivity()
{
	MojString retryActName;
	m_builderFactory->GetFolderRetrySyncActivityName(retryActName, m_folderId);

	// Cancel retry activity on successful sync
	ActivityPtr activity = m_activities->GetOrCreateActivity(retryActName);
	activity->SetEndAction(Activity::EndAction_Cancel);

	// We want to remove the retry activity last, only after we're sure that the
	// scheduled sync and watch activities have been created.
	activity->SetEndOrder(Activity::EndOrder_Last);
}

MojString SyncSession::GetCapabilityProvider()
{
	MojString s;
	s.assign("com.palm.pop.mail");
	return s;
}

MojString SyncSession::GetBusAddress()
{
	MojString s;
	s.assign("com.palm.pop");
	return s;
}

void SyncSession::StartSpinner()
{
	m_initialSync = IsAccountSync();
	BaseSyncSession::StartSpinner();
}

bool SyncSession::IsAccountSync()
{
	if (m_client.GetAccount().get()) {
		return m_client.GetAccount()->IsInitialSync();
	} else {
		// in error case, assume the account has been synced.
		return false;
	}
}

void SyncSession::SyncSessionComplete()
{
	MojLogInfo(m_log, "Sync session completed");
	if (m_completedSignal.connected()) {
		m_completedSignal.fire();
	}

	PopClient::AccountPtr account = m_client.GetAccount();
	if (account->IsInitialSync() && !PopErrors::IsNetworkError(m_accountError)) {
		MojLogDebug(m_log, "Account '%s' is initially synced.", AsJsonString(account->GetAccountId()).c_str());
		account->SetInitialSync(false);
		m_client.GetDatabaseInterface().UpdateAccountInitialSync(m_updateAccountSlot, account->GetAccountId(), account->IsInitialSync());
	} else {
		BaseSyncSession::SyncSessionComplete();
	}
}

MojErr SyncSession::UpdateAccountResponse(MojObject& response, MojErr err)
{
	BaseSyncSession::SyncSessionComplete();
	return MojErrNone;
}
