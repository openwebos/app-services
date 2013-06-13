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


#include "ImapClient.h"
#include "commands/ImapAccountFinder.h"
#include "data/ImapAccount.h"
#include "boost/shared_ptr.hpp"
#include "data/DatabaseInterface.h"
#include "ImapServiceApp.h"
#include "ImapPrivate.h"
#include "client/ImapSession.h"
#include "client/FileCacheClient.h"
#include "client/SyncSession.h"
#include "client/PowerManager.h"

#include "commands/DeleteAccountCommand.h"
#include "commands/DisableAccountCommand.h"
#include "commands/CheckOutboxCommand.h"
#include "commands/ImapSyncSessionCommand.h"
#include "commands/ScheduleRetryCommand.h"
#include "commands/SyncAccountCommand.h"
#include "commands/SyncFolderCommand.h"
#include "commands/UpdateAccountCommand.h"
#include "commands/SyncFolderCommand.h"
#include "commands/CheckOutboxCommand.h"
#include "commands/CheckDraftsCommand.h"
#include "commands/EnableAccountCommand.h"
#include "commands/UpdateAccountErrorCommand.h"
#include "client/DownloadListener.h"

using namespace boost;
using namespace std;

MojLogger ImapClient::s_log("com.palm.imap.client");

ImapClient::ImapClient(MojLunaService* service, ImapBusDispatcher* dispatcher, const MojObject& accountId, const boost::shared_ptr<DatabaseInterface>& dbInterface)
: BusClient(service),
  m_busDispatcher(dispatcher),
  m_dbInterface(dbInterface),
  m_fileCacheClient(new FileCacheClient(*this)),
  m_powerManager(new PowerManager(*this)),
  m_accountId(accountId),
  m_accountIdString(AsJsonString(accountId)),
  m_log(s_log),
  m_commandManager(new CommandManager(CommandManager::DEFAULT_MAX_CONCURRENT_COMMANDS, true)),
  m_state(State_NeedsAccountInfo),
  m_retryInterval(0),
  m_cleanupSessionCallbackId(0),
  m_getAccountRetries(0),
  m_smtpAccountEnabledSlot(this, &ImapClient::SmtpAccountEnabledResponse)
{
}

ImapClient::~ImapClient()
{
	
}

void ImapClient::LoginSuccess()
{
}

/**
 * Set the account using an account object from the database.
 */
void ImapClient::SetAccount(const boost::shared_ptr<ImapAccount>& account)
{
	MojLogTrace(m_log);

	if(account.get()) {
		m_account = account;
		HookUpFolderSessionAccounts();

		if(m_state == State_GettingAccount) {
			SetState(State_RunCommand);
			CheckQueue();
		}
	} else {
		MojLogCritical(m_log, "failed to get account %s from database", AsJsonString(GetAccountId()).c_str());

		if(!DisableAccountInProgress()) {
			// Reset so it can try again later
			SetState(State_NeedsAccountInfo);
		}

		// The first time, check the queue immediately so it tries again quickly.
		// After that, only if something else nudges it.
		if(m_getAccountRetries++ < 1) {
			MojLogInfo(m_log, "retrying getting account");
			CheckQueue();
		} else {
			CancelPendingCommands(ImapCommand::CancelType_NoAccount);
		}
	}
}

/**
 * Create a session for this client. If client already has a member account
 * will be automatically assigned into the session
 */
ImapClient::ImapSessionPtr ImapClient::CreateSession() {
	MojRefCountedPtr<ImapClient> clientPtr(this);
	ImapSessionPtr session(new ImapSession(clientPtr));

	session->SetDatabase(*m_dbInterface.get());
	session->SetFileCacheClient(*m_fileCacheClient.get());
	session->SetBusClient(*this);
	session->SetPowerManager(m_powerManager);

	if (m_account.get() != NULL) {
		session->SetAccount(m_account);
	}  // otherwise, rely on getAccount handler to set account for map sessions

	return session;
}

/**
 * Create session with folder id. account will be assigned if one exists in client
 */
ImapClient::ImapSessionPtr ImapClient::CreateSession(const MojObject& folderId) {
	MojLogInfo(m_log, "creating session for folderId %s", AsJsonString(folderId).c_str());
	ImapSessionPtr sess = CreateSession();
	sess->SetFolder(folderId);
	return sess;
}

void ImapClient::SetState(ClientState state)
{
	MojLogInfo(m_log, "setting client state from %s (%d) to %s (%d)",
			GetStateName(m_state), m_state, GetStateName(state), state);

	m_state = state;

	if(m_state != State_RunCommand) {
		m_commandManager->Pause();
	}

	// Update bus dispatcher so it knows that we're now active/inactive
	if(m_busDispatcher)
		m_busDispatcher->UpdateClientActive(this, IsActive());
}

void ImapClient::CheckQueue()
{
	MojLogInfo(m_log, "checking the queue for client %s. current state: %s (%d)", m_accountIdString.c_str(), GetStateName(m_state), m_state);

	switch(m_state)
	{
		case State_None:
		{
			MojLogCritical(m_log, "unable to run commands, no account available");
			break;
		}
		case State_NeedsAccountInfo:
		{
			SetState(State_GettingAccount);

			MojRefCountedPtr<ImapAccountFinder> command(new ImapAccountFinder(*this, m_accountId, true));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_GettingAccount:
		{
			break;
		}
		case State_LoginFailed:
		{
			break;
		}
		case State_RunCommand:
		{
			// Implicitly runs commands
			m_commandManager->Resume();
			break;
		}
		case State_TerminatingSessions:
		{
			break;
		}
		case State_DisableAccount:
		{
			MojLogNotice(m_log, "disabling account %s", AsJsonString(m_accountId).c_str());
			SetState(State_DisablingAccount);
			MojRefCountedPtr<DisableAccountCommand> command(new DisableAccountCommand(*this, m_accountEnabledMsg, m_payload));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_DisablingAccount:
		{
			break;
		}
		case State_DisabledAccount:
		{
			break;
		}
		case State_DeleteAccount:
		{
			MojLogNotice(m_log, "deleting account %s", AsJsonString(m_accountId).c_str());
			SetState(State_DeletingAccount);
			MojRefCountedPtr<DeleteAccountCommand> command(new DeleteAccountCommand(*this, m_deleteAccountMsg, m_payload));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_DeletingAccount:
		{
			break;
		}
		case State_DeletedAccount:
		{
			break;
		}
		default:
		{
			MojLogError(m_log, "CheckQueue in unknown state %d", m_state);
		}
	}
}

void ImapClient::RunCommandsInQueue()
{
	m_commandManager->RunCommands();
}

void ImapClient::SyncAccount(SyncParams syncParams)
{
	// TODO: debounce multiple sync requests
	MojRefCountedPtr<SyncAccountCommand> command(new SyncAccountCommand(*this, syncParams));
	m_commandManager->QueueCommand(command);
	CheckQueue();
}

void ImapClient::CheckOutbox(SyncParams syncParams)
{
	MojRefCountedPtr<CheckOutboxCommand> command(new CheckOutboxCommand(*this, syncParams));
	m_commandManager->QueueCommand(command);
	CheckQueue();
}

void ImapClient::CheckDrafts(const ActivityPtr& activity)
{
	MojRefCountedPtr<CheckDraftsCommand> command(new CheckDraftsCommand(*this, activity));
	m_commandManager->QueueCommand(command);
	CheckQueue();
}

void ImapClient::UploadSentEmails(const ActivityPtr& activity)
{
	MojObject folderId = GetAccount().GetSentFolderId();

	if(!IsValidId(folderId)) {
		folderId = GetAccount().GetInboxFolderId();
	}

	if(IsValidId(folderId)) {
		GetOrCreateSession(folderId)->UpSyncSentEmails(folderId, activity);
	}
}

void ImapClient::UploadDrafts(const ActivityPtr& activity)
{
	MojObject folderId = GetAccount().GetDraftsFolderId();

	if(!IsValidId(folderId)) {
		folderId = GetAccount().GetDraftsFolderId();
	}

	if(IsValidId(folderId)) {
		GetOrCreateSession(folderId)->UpSyncDrafts(folderId, activity);
	}
}

void ImapClient::EnableAccount(MojServiceMessage* msg)
{
	if(m_state == State_DisabledAccount) {
		// Reset state in case we're re-enabling an account
		SetState(State_NeedsAccountInfo);
	}

	// Run the EnableAccountCommand, which will create the default folders, call SMTP accountEnabled, and setup watches
	MojRefCountedPtr<EnableAccountCommand> command(new EnableAccountCommand(*this, msg));
	m_commandManager->QueueCommand(command, false);

	CheckQueue();
}

/**
 * Update account
 */
void ImapClient::UpdateAccount(const MojObject& accountId, const ActivityPtr& activity, bool credentialsChanged)
{
	// Don't do anything if it is in progress disabling the account.
	if(!DisableAccountInProgress())
	{
		fprintf(stderr, "Updating Account from ImapClient \n\n");
		m_activity = activity;
		MojRefCountedPtr<UpdateAccountCommand> command(new UpdateAccountCommand(*this, m_activity, credentialsChanged));
		m_commandManager->QueueCommand(command, false);
		CheckQueue();
	}
}

/**
 * Disable account is divided to two parts.
 * 1. Close the session.
 * 2. Remove folders and emails.
 */
void ImapClient::DisableAccount(MojServiceMessage* msg, MojObject& payload)
{
	fprintf(stderr, "Disabling Account from ImapClient \n\n");
	m_accountEnabledMsg = msg;
	m_payload = payload;

	if(!DisableAccountInProgress())
	{
		SetState(State_TerminatingSessions);

		if(m_folderSessionMap.empty())
		{
			SetState(State_DisableAccount);
			CheckQueue();
		}
		else
		{
			CleanupOneSession();
		}
	}
}

void ImapClient::CleanupOneSession()
{
	map<const MojObject, MojRefCountedPtr<ImapSession> >::iterator it = m_folderSessionMap.begin();
	((*it).second)->DisconnectSession();
}

/**
 * Delete account
 */
void ImapClient::DeleteAccount(MojServiceMessage* msg, MojObject& payload)
{
	fprintf(stderr, "Deleting Account from ImapClient \n\n");
	m_deleteAccountMsg = msg;
	m_payload = payload;
	SetState(State_DeleteAccount);
	CheckQueue();
}

// Queue sync folder command
void ImapClient::SyncFolder(const MojObject& folderId, SyncParams syncParams)
{
	MojRefCountedPtr<SyncFolderCommand> command(new SyncFolderCommand(*this, folderId, syncParams));
	m_commandManager->QueueCommand(command, false);
	CheckQueue();
}

// Ask the session to sync a folder
void ImapClient::StartFolderSync(const MojObject& folderId, SyncParams syncParams)
{
	MojLogDebug(m_log, "%s: preparing to sync folder", __PRETTY_FUNCTION__);

	// FIXME: check for account being deleted

	ImapSessionPtr session = GetOrCreateSession(folderId);

	// Connection-related activities go to the session
	session->AddActivities(syncParams.GetConnectionActivities());

	// Ask the session to queue a sync command if one isn't already pending
	session->SyncFolder(folderId, syncParams);
}

void ImapClient::DownloadPart(const MojObject& folderId, const MojObject& emailId, const MojObject& partId, const MojRefCountedPtr<DownloadListener>& listener)
{
	MojLogTrace(m_log);

	GetOrCreateSession(folderId)->DownloadPart(folderId, emailId, partId, listener, Command::HighPriority);
	CheckQueue();
}

void ImapClient::SearchFolder(const MojObject& folderId, const MojRefCountedPtr<SearchRequest>& searchRequest)
{
	GetOrCreateSession(folderId)->SearchFolder(folderId, searchRequest);
	CheckQueue();
}

ImapClient::ImapSessionPtr ImapClient::GetSession(const MojObject& folderId)
{
	if (m_folderSessionMap.find(folderId) != m_folderSessionMap.end()) {
		return m_folderSessionMap[folderId];
	}
	return NULL;
}

ImapClient::ImapSessionPtr ImapClient::GetOrCreateSession(const MojObject& folderId)
{
	if (m_folderSessionMap.find(folderId) == m_folderSessionMap.end()) {
		ImapSessionPtr session = CreateSession(folderId);
		// NOTE: assignment needs to be on a separate line so it doesn't create an
		// uninitialized map entry before CreateSession is run.
		m_folderSessionMap[folderId] = session;
	}
	return m_folderSessionMap[folderId];
}

/**
 * Iterates over folderSessionMap, assigning accounts into sessions lacking one. This
 * method should only be called by the setAccount(boost::shared_ptr<ImapAccount>& method);
 */
void ImapClient::HookUpFolderSessionAccounts() {
	if(m_account.get() == NULL) {
		throw MailException("account is null", __FILE__, __LINE__);
	}

	std::map< const MojObject, ImapSessionPtr >::iterator it;

	for (it = m_folderSessionMap.begin(); it != m_folderSessionMap.end(); ++it) {
		it->second->SetAccount(m_account);
	}
}

ImapClient::SyncSessionPtr ImapClient::GetSyncSession(const MojObject& folderId)
{
	if(m_syncSessions.find(folderId) != m_syncSessions.end()) {
		const SyncSessionPtr& syncSession = m_syncSessions[folderId];

		if(syncSession->IsRunning()) {
			return syncSession;
		}
	}

	return NULL;
}

ImapClient::SyncSessionPtr ImapClient::GetOrCreateSyncSession(const MojObject& folderId)
{
	SyncSessionPtr& syncSession = m_syncSessions[folderId];

	if(syncSession.get() == NULL) {
		syncSession.reset(new SyncSession(*this, folderId));
	}

	return syncSession;
}

void ImapClient::StartSyncSession(const MojObject& folderId, const vector<ActivityPtr>& activities)
{
	SyncSessionPtr syncSession = GetOrCreateSyncSession(folderId);

	BOOST_FOREACH(const ActivityPtr& activity, activities) {
		syncSession->AdoptActivity(activity);
	}

	syncSession->RequestStart();
}

void ImapClient::AddToSyncSession(const MojObject& folderId, ImapSyncSessionCommand* command)
{
	assert(command);

	SyncSessionPtr syncSession = GetOrCreateSyncSession(folderId);

	// Need to set this before calling RegisterCommand
	// since it may be ready to start the command immediately
	command->SetSyncSession(syncSession);

	syncSession->RegisterCommand(command);
}

void ImapClient::RequestSyncSession(const MojObject& folderId, ImapSyncSessionCommand* command, MojSignal<>::SlotRef slot)
{
	assert(command);

	SyncSessionPtr syncSession = GetOrCreateSyncSession(folderId);

	// Need to set this before calling RegisterCommand
	// since it may be ready to start the command immediately
	command->SetSyncSession(syncSession);

	syncSession->RegisterCommand(command, slot);
}


// TODO: Error handling. Probably remove the functions below

void ImapClient::CommandComplete(Command* command)
{
	m_commandManager->CommandComplete(command);

	// Check if we're idle; if so, report to dispatcher
	if(m_busDispatcher)
		m_busDispatcher->UpdateClientActive(this, IsActive());
}


void ImapClient::CommandFailure(ImapCommand* command)
{
	MojLogTrace(m_log);
//	m_failedCommandTotal++;
//	m_successfulCommandTotal--;
	CommandComplete(command);
}

void ImapClient::CommandFailure(ImapCommand* command, const std::exception& e)
{
	MojLogTrace(m_log);

	MojLogError(m_log, "command failure in %s, exception: %s", command->Describe().c_str(), e.what());
	CommandFailure(command);
}

const char* ImapClient::GetStateName(ClientState state)
{
	switch(state) {
	case State_None: return "None";
	case State_NeedsAccountInfo: return "NeedsAccountInfo";
	case State_GettingAccount: return "GettingAccount";
	case State_LoginFailed: return "LoginFailed";
	case State_RunCommand: return "RunCommand";
	case State_TerminatingSessions: return "TerminatingSessions";
	case State_DisableAccount: return "DisableAccount";
	case State_DisablingAccount: return "DisablingAccount";
	case State_DisabledAccount: return "DisabledAccount";
	case State_DeleteAccount: return "DeleteAccount";
	case State_DeletingAccount: return "DeletingAccount";
	case State_DeletedAccount: return "DeletedAccount";
	}

	return "unknown";
}

void ImapClient::Status(MojObject& status)
{
	MojErr err;
	MojObject sessions(MojObject::TypeArray);

	if(!m_folderSessionMap.empty()) {
		std::map<const MojObject, ImapSessionPtr >::const_iterator it;

		for(it = m_folderSessionMap.begin(); it != m_folderSessionMap.end(); ++it) {
			MojObject sessionStatus;

			if(it->second.get()) {
				it->second->Status(sessionStatus);
				err = sessions.push(sessionStatus);
				ErrorToException(err);
			}
		}
	}

	if(!m_syncSessions.empty()) {
		MojObject syncSessions(MojObject::TypeArray);

		std::map<const MojObject, SyncSessionPtr>::const_iterator it;

		for(it = m_syncSessions.begin(); it != m_syncSessions.end(); ++it) {
			MojObject syncSessionStatus;
			it->second->Status(syncSessionStatus);
			syncSessions.push(syncSessionStatus);
		}

		status.put("syncSessions", syncSessions);
	}

	err = status.put("accountId", m_accountId);
	ErrorToException(err);
	err = status.put("sessions", sessions);
	ErrorToException(err);
	err = status.putString("clientState", GetStateName(m_state));
	ErrorToException(err);

	if(m_account.get()) {
		MojObject accountStatus;

		err = accountStatus.putString("templateId", m_account->GetTemplateId().c_str());
		ErrorToException(err);

		const EmailAccount::AccountError& error = m_account->GetError();
		if(error.errorCode != MailError::NONE) {
			err = accountStatus.put("errorCode", error.errorCode);
			ErrorToException(err);

			err = accountStatus.putString("errorText", error.errorText.c_str());
			ErrorToException(err);
		}

		const EmailAccount::RetryStatus& retry = m_account->GetRetry();
		if(retry.GetInterval() > 0) {
			MojObject retryStatus;
			err = retryStatus.put("interval", retry.GetInterval());
			ErrorToException(err);

			err = accountStatus.put("retry", retryStatus);
			ErrorToException(err);
		}

		MojObject folderIdStatus;

		err = folderIdStatus.put("inboxFolderId", m_account->GetInboxFolderId());
		ErrorToException(err);

		err = folderIdStatus.put("draftsFolderId", m_account->GetDraftsFolderId());
		ErrorToException(err);

		err = folderIdStatus.put("sentFolderId", m_account->GetSentFolderId());
		ErrorToException(err);

		err = folderIdStatus.put("outboxFolderId", m_account->GetOutboxFolderId());
		ErrorToException(err);

		err = folderIdStatus.put("trashFolderId", m_account->GetTrashFolderId());
		ErrorToException(err);

		err = accountStatus.put("folders", folderIdStatus);
		ErrorToException(err);

		err = accountStatus.put("syncFrequencyMins", m_account->GetSyncFrequencyMins());

		err = status.put("accountInfo", accountStatus);
		ErrorToException(err);
	}

	if(m_commandManager->GetActiveCommandCount() > 0 || m_commandManager->GetPendingCommandCount() > 0) {
		MojObject commandManagerStatus;
		m_commandManager->Status(commandManagerStatus);
		err = status.put("clientCommands", commandManagerStatus);
		ErrorToException(err);
	}
}

void ImapClient::SessionDisconnected(MojObject& folderId)
{
	// Cleanup sync session (if any)
	SyncSessionPtrMap::iterator it = m_syncSessions.find(folderId);
	if(it != m_syncSessions.end()) {
		SyncSessionPtr syncSession = it->second;
		if(syncSession.get() && syncSession->IsFinished()) {
			m_syncSessions.erase(it);
		}
	}

	if(m_state == State_TerminatingSessions) {
		// Remove the closed session.

		if(m_folderSessionMap.find(folderId) != m_folderSessionMap.end()) {
			m_pendingDeletefolderSessions.push_back(m_folderSessionMap[folderId]);
			m_folderSessionMap.erase(folderId);
		}

		if(m_pendingDeletefolderSessions.size() == 1) {
			m_cleanupSessionCallbackId = g_idle_add(&ImapClient::CleanupSessionsCallback, gpointer(this));
		}

	} else {
		m_busDispatcher->UpdateClientActive(this, IsActive());
	}
}

void ImapClient::HandleSessionCleanup()
{
	m_cleanupSessionCallbackId = 0;
	m_pendingDeletefolderSessions.clear();
	if(m_state == State_TerminatingSessions) {
		// Delete the account data when all sessions are closed.
		if(m_folderSessionMap.empty())
		{
			SetState(State_DisableAccount);
			CheckQueue();
		}
		else
		{
			CleanupOneSession();
		}
	}
}

gboolean ImapClient::CleanupSessionsCallback(gpointer data)
{
	ImapClient* client = static_cast<ImapClient*>(data);
	assert(data);
	client->HandleSessionCleanup();
	return false;
}

void ImapClient::DisabledAccount()
{
	MojLogNotice(m_log, "account %s disabled successfully", AsJsonString(m_accountId).c_str());
	SetState(State_DisabledAccount);
}

void ImapClient::SendSmtpAccountEnableRequest(bool enabling)
{
	MojObject payload;
	MojErr err;
	err = payload.put("accountId", m_accountId);
	ErrorToException(err);
	err = payload.put("enabled", enabling);
	ErrorToException(err);

	// Cancel any existing outstanding request to avoid crashing
	// FIXME move this into a command
	m_smtpAccountEnabledSlot.cancel();
	SendRequest(m_smtpAccountEnabledSlot, "com.palm.smtp", "accountEnabled", payload);
}

MojErr ImapClient::SmtpAccountEnabledResponse(MojObject& response, MojErr err)
{
	if (err)
		return m_accountEnabledMsg->replyError(err);

	return m_accountEnabledMsg->replySuccess();
}

void ImapClient::DeletedAccount()
{
	SetState(State_DeletedAccount);
}

bool ImapClient::DisableAccountInProgress()
{
	return m_state == State_TerminatingSessions ||
		   m_state == State_DisableAccount ||
		   m_state == State_DisablingAccount ||
		   m_state == State_DisabledAccount ||
		   m_state == State_DeleteAccount ||
		   m_state == State_DeletingAccount ||
		   m_state == State_DeletedAccount;
}

void ImapClient::WakeupIdle(const MojObject& folderId)
{
	if(m_folderSessionMap.find(folderId) != m_folderSessionMap.end()) {
		m_folderSessionMap[folderId]->WakeupIdle();
	}
}

void ImapClient::UpdateAccountStatus(const MailError::ErrorInfo& error)
{
	// Set updated error on account
	if(m_account.get()) {
		m_account->SetError(error);
	}

	MojRefCountedPtr<UpdateAccountErrorCommand> command(new UpdateAccountErrorCommand(*this, error));
	m_commandManager->RunCommand(command);
}

bool ImapClient::IsPushReady(const MojObject& folderId)
{
	const ImapSessionPtr& session = this->GetSession(folderId);
	if(session.get() && session->IsPushEnabled(folderId)) {
		return true;
	} else {
		return false;
	}
}

// Debug method for testing
void ImapClient::MagicWand(MojServiceMessage* msg, const MojObject& payload)
{
	MojErr err;
	MojString command;

	err = payload.getRequired("command", command);
	ErrorToException(err);

	vector<ImapSessionPtr> sessions;
	MojObject folderId;

	if(payload.get("folderId", folderId)) {
		ImapSessionPtr session = GetSession(folderId);
		if(session.get())
			sessions.push_back(session);
	} else {
		map<const MojObject, ImapSessionPtr>::iterator it;
		for(it = m_folderSessionMap.begin(); it != m_folderSessionMap.end(); ++it) {
			const ImapSessionPtr& session = it->second;
			sessions.push_back(session);
		}
	}

	if(command == "disconnect") {
		BOOST_FOREACH(const ImapSessionPtr& session, sessions) {
			fprintf(stderr, "disconnecting\n");
			session->DisconnectSession();
		}

		msg->replySuccess();
	} else {
		msg->replyError(MojErrUnknown, "no such command");
	}
}

void ImapClient::UpdateSessionActive(ImapSession* session, bool isActive)
{
	if(m_busDispatcher)
		m_busDispatcher->UpdateClientActive(this, IsActive());
}

bool ImapClient::IsActive()
{
	// Don't shut down if any commands are running
	if(m_commandManager->GetActiveCommandCount() > 0) {
		return true;
	}

	// If the account is disabled, it's not active
	switch(m_state) {
	case State_DisabledAccount:
	case State_DeletedAccount:
	case State_LoginFailed:
		return false;
	default:
		break;
	}

	// Don't shut down if we have commands queued
	if(m_commandManager->GetPendingCommandCount() > 0) {
		return true;
	}

	// Check if any sessions are active
	map<const MojObject, ImapSessionPtr>::iterator it;
	for(it = m_folderSessionMap.begin(); it != m_folderSessionMap.end(); ++it) {
		const ImapSessionPtr& session = it->second;
		if(session->IsActive()) {
			return true;
		}
	}

	return false;
}

NetworkStatusMonitor& ImapClient::GetNetworkStatusMonitor() const
{
	return m_busDispatcher->GetNetworkStatusMonitor();
}

void ImapClient::CancelPendingCommands(ImapCommand::CancelType cancelType)
{
	BOOST_FOREACH(const MojRefCountedPtr<Command>& pendingCommand, m_commandManager->GetPendingCommandIterators()) {
		// Assume it's an ImapCommand
		ImapCommand* imapCommand = static_cast<ImapCommand*>(pendingCommand.get());

		bool cancelled = imapCommand->Cancel(cancelType);

		if (cancelled) {
			MojLogInfo(m_log, "cancelled command %s (%p)", imapCommand->Describe().c_str(), imapCommand);
		}
	}
}
