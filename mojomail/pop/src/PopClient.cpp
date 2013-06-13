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

#include "PopClient.h"
#include "PopDefs.h"
#include "commands/AccountFinderCommand.h"
#include "commands/DownloadPartCommand.h"
#include "commands/MoveEmailsCommand.h"
#include "commands/MovePopEmailsCommand.h"
#include "commands/PopAccountDisableCommand.h"
#include "commands/PopAccountDeleteCommand.h"
#include "commands/PopAccountEnableCommand.h"
#include "commands/PopAccountUpdateCommand.h"
#include "commands/SyncAccountCommand.h"
#include "commands/SyncFolderCommand.h"
#include "commands/ScheduleRetryCommand.h"
#include "data/PopFolder.h"
#include "data/PopFolderAdapter.h"
#include "data/PopEmailAdapter.h"
#include "data/PopAccountAdapter.h"
#include "PopBusDispatcher.h"

using namespace boost;
using namespace std;

MojLogger PopClient::s_log("com.palm.pop.client");

PopClient::PopClient(DBInterfacePtr dbInterface, PopBusDispatcher* dispatcher, const MojObject& accountId, MojLunaService* service)
: BusClient(service),
  m_deleted(false),
  m_dbInterface(dbInterface),
  m_state(State_None),
  m_startedCommandTotal(0),
  m_successfulCommandTotal(0),
  m_failedCommandTotal(0),
  m_capabilityProvider("com.palm.pop.mail"),
  m_busAddress("com.palm.pop"),
  m_commandManager(new CommandManager(1)),
  m_powerManager(new PowerManager(*this)),
  m_builderFactory(new ActivityBuilderFactory()),
  m_fileCacheClient(new FileCacheClient(*this)),
  m_accountId(accountId),
  m_accountIdString(AsJsonString(accountId)),
  m_popBusDispatcher(dispatcher)
{

}

PopClient::~PopClient()
{
	// Need to shut down the power manager to make sure nothing tries to
	// create/end power activities during the destruction process.
	if(m_powerManager.get()) {
		m_powerManager->Shutdown();
	}
}

PopClient::PopSessionPtr PopClient::CreateSession() {
	m_session.reset(new PopSession());
	m_session->SetDatabaseInterface(m_dbInterface);
	m_session->SetPowerManager(m_powerManager);
	m_session->SetFileCacheClient(m_fileCacheClient);
	m_session->SetClient(*this);

	return m_session;
}

DatabaseInterface& PopClient::GetDatabaseInterface()
{
	return *m_dbInterface.get();
}

PopClient::AccountPtr PopClient::GetAccount()
{
	MojLogTrace(s_log);

	return m_account;
}

void PopClient::SetAccount(MojObject& accountId)
{
	MojLogTrace(s_log);

	assert(m_state == State_None);
	assert(!accountId.null());
	assert(!accountId.undefined());

	m_accountId = accountId;
	m_builderFactory->SetAccountId(m_accountId);
}

void PopClient::SetAccount(AccountPtr account)
{
	MojLogTrace(s_log);

	if (account.get()) {
		m_account = account;
		m_session->SetAccount(account);
		m_builderFactory->SetAccountId(m_account->GetAccountId());

		if (m_account->IsDeleted()) {
			MojLogInfo(s_log, "Account %s is being deleted", AsJsonString(m_account->GetAccountId()).c_str());
		} else if (m_state == State_LoadingAccount || m_state == State_NeedAccount || m_state == State_None) {
			m_state = State_OkToRunCommands;
		}
		CheckQueue();
	} else {
		MojLogCritical(s_log, "failed to find account in database");
	}
}

void PopClient::FindAccountFailed(const MojObject& accountId)
{
	MojLogError(s_log, "Failed to load account '%s'", AsJsonString(accountId).c_str());

	m_state = State_OkToRunCommands;
	CheckQueue();
}

PopClient::SyncSessionPtr PopClient::GetOrCreateSyncSession(const MojObject& folderId)
{
	if (!m_syncSession.get()) {
		// since POP transport only syncs inbox, we will create one sync session for inbox
		m_syncSession.reset(new SyncSession(*this, folderId));
		m_session->SetSyncSession(m_syncSession);
	}

	return m_syncSession;
}

void PopClient::DeleteAccount(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(s_log);

	if (m_state == State_AccountDeleted) {
		// should not delete the same account twice
		return;
	}

	m_state = State_AccountDeleted;
	m_session->DeleteAccount(msg);

	// TODO: wait until all the current commands are done/there are no pending commands for client

	CommandManager::CommandPtr deleteAccount(new PopAccountDeleteCommand(*this, m_dbInterface, payload, msg));
	QueueCommand(deleteAccount, true);

	CheckQueue();
}

void PopClient::DisableAccount(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, "disable account");

	if (m_state == State_AccountDisabled) {
		// should not disable account twice
		return;
	}

	m_state = State_AccountDisabled;
	m_session->DisableAccount(msg);

	// TODO: wait until all the current commands are done/there are no pending commands for client

	CommandManager::CommandPtr disableAccount(new PopAccountDisableCommand(*this, m_dbInterface, payload, msg));
	QueueCommand(disableAccount, true);

	CheckQueue();
}

void PopClient::AccountDeleted()
{
	m_deleted = true;
	m_popBusDispatcher->SetupCleanupCallback();
}

bool PopClient::IsAccountDeleted()
{
	return m_deleted;
}

void PopClient::Cleanup()
{
	MojLogTrace(s_log);
}


void PopClient::EnableAccount(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, "Enable pop account %s", AsJsonString(m_accountId).c_str());

	if (m_state == State_None || m_state == State_AccountDisabled) {
		if (m_state == State_AccountDisabled) {
			// reset the client and sessions states upon account re-enabling.
			Reset();
		}

		m_state = State_NeedAccount;
		m_commandManager->Pause();
	}

	m_session->EnableAccount();

	MojLogInfo(s_log, "queuing command to enable POP account (create folders + call accountEnable on SMTP");
	CommandManager::CommandPtr enableAccount(new PopAccountEnableCommand(*this, msg, payload));
	QueueCommand(enableAccount, false);

	MojLogInfo(s_log, "queuing sync account command");
	CommandManager::CommandPtr syncAccount(new SyncAccountCommand(*this, payload));
	QueueCommand(syncAccount, false);

	CheckQueue();
}

void PopClient::UpdateAccount(MojObject& payload, bool credentialsChanged)
{
	MojLogTrace(s_log);

	if (m_state == State_AccountDisabled || m_state == State_AccountDeleted) {
		return;
	}

	if (m_state == State_None || m_state == State_NeedAccount) {
		m_state = State_NeedAccount;
		m_commandManager->Pause();
	}

	CommandManager::CommandPtr updateAccount(new PopAccountUpdateCommand(*this, payload, credentialsChanged));
	QueueCommand(updateAccount, true);

	CheckQueue();
}

void PopClient::SyncAccount(MojObject& payload)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, "Syncing pop account %s", AsJsonString(m_accountId).c_str());

	if (m_state > State_OkToRunCommands) {
		return;
	}

	if (m_state == State_None) {
		m_state = State_NeedAccount;
		m_commandManager->Pause();
	}

	// sync folders first
	MojLogInfo(s_log, "Creating command to sync folders");
	CommandManager::CommandPtr syncAccount(new SyncAccountCommand(*this, payload));
	QueueCommand(syncAccount, false);

	// run the command queue
	CheckQueue();
}

void PopClient::SyncFolder(MojObject& payload)
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, "Syncing folder in account %s", AsJsonString(m_accountId).c_str());

	if (m_state > State_OkToRunCommands) {
		return;
	}

	if (m_state == State_None || m_state == State_NeedAccount) {
		m_state = State_NeedAccount;
		m_commandManager->Pause();
	}

	// sync folder
	CommandManager::CommandPtr syncFolder(new SyncFolderCommand(*this, payload));
	QueueCommand(syncFolder, false);

	// run the command queue
	CheckQueue();
}

void PopClient::MoveOutboxFolderEmailsToSentFolder(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(s_log);

	if (m_state == State_AccountDisabled || m_state == State_AccountDeleted) {
		return;
	}

	if (m_state == State_None || m_state == State_NeedAccount) {
		m_state = State_NeedAccount;
		m_commandManager->Pause();
	}

	MojLogInfo(s_log, "PopClient: Moving outbox folder emails to sent folder");
	CommandManager::CommandPtr moveEmails(new MoveEmailsCommand(*this, msg, payload));
	QueueCommand(moveEmails);

	CheckQueue();
}

void PopClient::MoveEmail(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(s_log);

	if (m_state == State_AccountDisabled || m_state == State_AccountDeleted) {
		return;
	}

	if (m_state == State_None || m_state == State_NeedAccount) {
		m_state = State_NeedAccount;
		m_commandManager->Pause();
	}

	MojLogInfo(s_log, "PopClient: Moving email");
	CommandManager::CommandPtr moveEmail(new MovePopEmailsCommand(*this, msg, payload));
	QueueCommand(moveEmail);

	CheckQueue();
}

void PopClient::DownloadPart(const MojObject& folderId, const MojObject& emailId, const MojObject& partId, boost::shared_ptr<DownloadListener>& listener)
{
	MojLogTrace(s_log);

	if (m_state == State_AccountDisabled || m_state == State_AccountDeleted) {
		return;
	}

	if (m_state == State_None || m_state == State_NeedAccount) {
		m_state = State_NeedAccount;
		m_commandManager->Pause();
	}

	MojLogInfo(s_log, "queueing download of attachment: %s", AsJsonString(partId).c_str());
	CommandManager::CommandPtr downloadPart(new DownloadPartCommand(*this, folderId, emailId, partId, listener));
	QueueCommand(downloadPart, true);

	// run the command queue
	CheckQueue();
}

void PopClient::ScheduleRetrySync(const EmailAccount::AccountError& err) {
	MojObject inboxId = m_account->GetInboxFolderId();
	MojLogInfo(s_log, "scheduling a retry sync for folder: %s", AsJsonString(inboxId).c_str());
	CommandManager::CommandPtr scheduleSync(new ScheduleRetryCommand(*this, inboxId, err));
	RunCommand(scheduleSync);
}

const char* PopClient::GetStateName(State state)
{
	switch(state) {
	case State_None: return "None";
	case State_NeedAccount: return "NeedAccount";
	case State_LoadingAccount: return "LoadingAccount";
	case State_OkToRunCommands: return "OkToRunCommands";
	case State_AccountDisabled: return "AccountDisabled";
	case State_AccountDeleted: return "AccountDeleted";
	}

	return "unknown";
}

void PopClient::CheckQueue()
{
	MojLogInfo(s_log, "PopClient: checking the queue. current state: %s (%d)", GetStateName(m_state), m_state);

	switch(m_state) {
		case State_None: {
			break;
		}
		case State_NeedAccount: {
			CommandManager::CommandPtr accountFinder(new AccountFinderCommand(*this, m_accountId));
			RunCommand(accountFinder);
			m_state = State_LoadingAccount;

			break;
		}
		case State_LoadingAccount: {
			break;
		}
		case State_OkToRunCommands: {
			MojLogInfo(s_log, "PopClient: %i command(s) to run in queue", m_commandManager->GetPendingCommandCount());
			m_commandManager->Resume();
			m_commandManager->RunCommands();

			break;
		}
		default: break;
	}
}

void PopClient::CommandComplete(Command* command)
{
	m_commandManager->CommandComplete(command);

	if (m_commandManager->GetActiveCommandCount() == 0 && m_commandManager->GetPendingCommandCount() == 0) {
		// no more commands to run

		if (m_session->IsSessionShutdown()) {
			// if the session is not idle, inform bus dispatcher about the inactiveness
			// of this client.
			m_popBusDispatcher->PrepareShutdown();
		}
	}

	CheckQueue();
}

void PopClient::CommandFailed(Command* command, MailError::ErrorCode errCode, const std::exception& exc)
{
	// TODO: log error to RDX report
	MojLogError(s_log, "Error running command: %s", exc.what());

	CommandComplete(command);
}

void PopClient::QueueCommand(CommandManager::CommandPtr command, bool runImmediately)
{
	m_commandManager->QueueCommand(command, runImmediately);
}

void PopClient::RunCommand(MojRefCountedPtr<Command> command)
{
	m_commandManager->RunCommand(command);
}

void PopClient::Reset()
{
	// initializes counters
	m_startedCommandTotal = 0;
	m_successfulCommandTotal = 0;
	m_failedCommandTotal = 0;

	// reset untility objects
	m_commandManager.reset(new CommandManager(1));
	m_powerManager.reset(new PowerManager(*this));
	m_builderFactory.reset(new ActivityBuilderFactory());
	m_fileCacheClient.reset(new FileCacheClient(*this));

	// create POP session and reset sync session and account
	CreateSession();
	m_syncSession.reset();
	m_account.reset();
}

bool PopClient::IsActive()
{
	return m_commandManager->GetActiveCommandCount() > 0
			|| m_commandManager->GetPendingCommandCount() > 0
			|| !m_session->IsSessionShutdown();
}

void PopClient::SessionShutdown()
{
	if (m_commandManager->GetActiveCommandCount() == 0
			&& m_commandManager->GetPendingCommandCount() == 0) {
		m_popBusDispatcher->PrepareShutdown();
	}
}

void PopClient::Status(MojObject& status) const
{
	MojErr err;

	err = status.put("accountId", m_accountId);
	ErrorToException(err);

	err = status.putString("state", GetStateName(m_state));
	ErrorToException(err);

	if(m_commandManager->GetActiveCommandCount() > 0 || m_commandManager->GetPendingCommandCount() > 0) {
		MojObject cmStatus;
		m_commandManager->Status(cmStatus);

		err = status.put("commandManager", cmStatus);
		ErrorToException(err);
	}

	if(m_session.get()) {
		MojObject sessionStatus;
		m_session->Status(sessionStatus);

		err = status.put("session", sessionStatus);
		ErrorToException(err);
	}
}
