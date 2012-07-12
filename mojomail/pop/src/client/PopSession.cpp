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

#include <boost/lexical_cast.hpp>

#include "client/PopSession.h"
#include "client/CommandManager.h"
#include "client/FileCacheClient.h"
#include "data/PopAccount.h"
#include "commands/AutoDownloadCommand.h"
#include "commands/CheckTlsCommand.h"
#include "commands/ConnectCommand.h"
#include "commands/CreateUidMapCommand.h"
#include "commands/DownloadEmailHeaderCommand.h"
#include "commands/FetchEmailCommand.h"
#include "commands/LogoutCommand.h"
#include "commands/NegotiateTlsCommand.h"
#include "commands/PasswordCommand.h"
#include "commands/SyncEmailsCommand.h"
#include "commands/UpdateAccountStatusCommand.h"
#include "commands/UserCommand.h"
#include "PopDefs.h"
#include "PopErrors.h"

#include <iostream>

MojLogger PopSession::s_log("com.palm.pop.session");

PopSession::PopSession()
: m_log(s_log),
  m_commandManager(new CommandManager(1)),
  m_uidMap(new UidMap()),
  m_client(NULL),
  m_requestManager(new RequestManager()),
  m_reconnect(false),
  m_canShutdown(true),
  m_state(State_NeedsConnection)
{
}

PopSession::PopSession(PopAccountPtr account)
: m_log(s_log),
  m_commandManager(new CommandManager(1)),
  m_account(account),
  m_uidMap(new UidMap()),
  m_client(NULL),
  m_requestManager(new RequestManager()),
  m_canShutdown(true),
  m_state(State_NeedsConnection)
{
}

PopSession::~PopSession()
{
}

void PopSession::SetAccount(PopAccountPtr account)
{
	PendAccountUpdates(account);
	if (!m_syncSession.get() || !m_syncSession->IsActive()) {
		// account changes can only be applied when there is no active folder sync
		ApplyAccountUpdates();
	}
}

void PopSession::SetClient(PopClient& client)
{
	m_client = &client;
}

void PopSession::SetConnection(MojRefCountedPtr<SocketConnection> connection)
{
	m_connection = connection;

	if(connection.get()) {
		m_inputStream = connection->GetInputStream();
		m_outputStream = connection->GetOutputStream();
	}
}

void PopSession::SetDatabaseInterface(boost::shared_ptr<DatabaseInterface> dbInterface)
{
	m_dbInterface = dbInterface;
}

void PopSession::SetPowerManager(MojRefCountedPtr<PowerManager> powerManager)
{
	m_powerManager = powerManager;
}

void PopSession::SetSyncSession(MojRefCountedPtr<SyncSession> syncSession)
{
	m_syncSession = syncSession;
}

void PopSession::SetError(MailError::ErrorCode errCode, const std::string& errMsg)
{
	if (errCode != MailError::NONE) {
		MojLogError(m_log, "------ Setting account error: [%d]%s", (int)errCode, errMsg.c_str());
	}

	m_accountError.errorCode = errCode;
	m_accountError.errorText = errMsg;
	if (m_syncSession.get() && (m_syncSession->IsActive() || m_syncSession->IsStarting())) {
		m_syncSession->SetAccountError(m_accountError);
	}
}

void PopSession::ResetError()
{
	SetError(MailError::VALIDATED, "");
}

const PopSession::PopAccountPtr& PopSession::GetAccount() const
{
	return m_account;
}

DatabaseInterface& PopSession::GetDatabaseInterface()
{
	return *m_dbInterface.get();
}

MojRefCountedPtr<PowerManager> PopSession::GetPowerManager()
{
	return m_powerManager;
}

MojRefCountedPtr<SyncSession> PopSession::GetSyncSession()
{
	return m_syncSession;
}

boost::shared_ptr<FileCacheClient> PopSession::GetFileCacheClient()
{
	if(m_fileCacheClient.get())
		return m_fileCacheClient;
	else
		throw MailException("no file cache client configured", __FILE__, __LINE__);
}

MojRefCountedPtr<SocketConnection> PopSession::GetConnection()
{
	return m_connection;
}

InputStreamPtr& PopSession::GetInputStream()
{
	if ( m_inputStream.get() == NULL ) {
		throw MailNetworkDisconnectionException("Connection is not available", __FILE__, __LINE__);
	}
	return m_inputStream;
}

OutputStreamPtr& PopSession::GetOutputStream()
{
	if ( m_outputStream.get() == NULL ) {
		throw MailNetworkDisconnectionException("Connection is not available", __FILE__, __LINE__);
	}
	return m_outputStream;
}

LineReaderPtr& PopSession::GetLineReader()
{
	if(m_lineReader.get() == NULL)
		m_lineReader.reset(new LineReader(GetInputStream()));

	return m_lineReader;
}

const boost::shared_ptr<UidMap>& PopSession::GetUidMap() const
{
	return m_uidMap;
}

bool PopSession::HasLoginError()
{
	return PopErrors::IsLoginError(m_accountError);
}

bool PopSession::HasRetryNetworkError()
{
	return PopErrors::IsRetryNetworkError(m_accountError);
}

bool PopSession::HasSSLNetworkError()
{
	return PopErrors::IsSSLNetworkError(m_accountError);
}

bool PopSession::HasNetworkError()
{
	return PopErrors::IsNetworkError(m_accountError);
}

bool PopSession::HasRetryError()
{
	return PopErrors::IsRetryError(m_accountError);
}

bool PopSession::IsSessionShutdown()
{
	return m_canShutdown;
}

bool PopSession::HasRetryAccountError()
{
	// Hotmail, Live, or MSN account can has temporary account unavailable
	// error, which means that the account is temporarily locked due to too
	// frequent account access.  In this case of error, we can schedule a sync
	// to happen in 15-30 minutes.
	return PopErrors::IsRetryAccountError(m_accountError);
}

void PopSession::Connected()
{
	if (m_account->GetEncryption() == PopAccount::SOCKET_ENCRYPTION_TLS) {
		m_state = State_CheckTlsSupport;
	} else {
		m_state = State_UsernameRequired;
	}

	// if there was an connection error, reset it
	if (HasNetworkError()) {
		ResetError();
	}

	MojLogInfo(m_log, "Connected to POP server. Need to send username.");

	CheckQueue();
}

void PopSession::ConnectFailure(MailError::ErrorCode errCode, const std::string& errMsg)
{
	// skip to not_ok_to_sync state
	m_state = State_CancelPendingCommands;
	SetError(errCode, errMsg);
	CheckQueue();
}

void PopSession::TlsSupported()
{
	m_state = State_NegotiateTlsConnection;
	CheckQueue();
}

void PopSession::TlsNegotiated()
{
	m_state = State_UsernameRequired;
	CheckQueue();
}

void PopSession::TlsFailed()
{
	MojLogInfo(m_log, "Failed to turn on TLS on socket");
	CheckQueue();
}

void PopSession::UserOk()
{
	m_state = State_PasswordRequired;
	CheckQueue();
}

void PopSession::LoginSuccess()
{
	if (HasLoginError() || HasRetryAccountError()) {
		// clear account's login error since we pass in a non-error code and message.
		MojLogInfo(m_log, "Clearing login failure status from email account");
		// updating 'm_accountError' will be done inside the following call.
		UpdateAccountStatus(m_account, MailError::VALIDATED, "");
	}

	m_state = State_NeedUidMap;
	CheckQueue();
}

void PopSession::LoginFailure(MailError::ErrorCode errorCode, const std::string& errorMsg)
{
	// update account status
	MojLogInfo(m_log, "Login failure for account '%s': %s", AsJsonString(m_account->GetAccountId()).c_str(), errorMsg.c_str());
	// updating 'm_accountError' will be done inside the following call.
	PersistFailure(errorCode, errorMsg);

	m_state = State_CancelPendingCommands;
	CheckQueue();
}

void PopSession::GotUidMap()
{
	m_state = State_OkToSync;
	MojLogInfo(m_log, "UID map size: %d", m_uidMap->Size());
	CheckQueue();
}

void PopSession::SyncCompleted()
{
	if (m_state == State_SyncingEmails) {
		m_state = State_OkToSync;
	}
	ResetError();
	CheckQueue();
}

void PopSession::LogoutDone()
{
	// disconnect the connection
	MojLogInfo(m_log, "Shutting down connection after successful log out");
	ShutdownConnection();
}

void PopSession::Disconnected()
{
	MojLogInfo(m_log, "Disconnected");

	// inform POP client that the current session has been shut down.
	m_canShutdown = true;
	if (m_client != NULL) {
		m_client->SessionShutdown();
	}
}

void PopSession::ShutdownConnection() {
	// shutting down connection
	if (m_connection.get()) {
		m_connection->Shutdown();
	}

	// TODO: should do the following actions in a callback function to socket's WatchClosed() function
	// reset line reader, input/output streem and connection.
	m_lineReader.reset(NULL);
	m_inputStream.reset(NULL);
	m_outputStream.reset(NULL);
	m_connection.reset(NULL);
	// reset UID map
	m_uidMap.reset(new UidMap());

	// once disconnected, change the state to need connection
	m_state = State_NeedsConnection;

	// mark the pop session as disconnected
	if(m_reconnect && m_commandManager->GetPendingCommandCount()>0) {
		m_reconnect = false;
		CheckQueue();
	} else {
		Disconnected();
	}
}

void PopSession::Reconnect() {
	// re-establish the connection
	m_reconnect = true;
	if (m_syncSession.get()) {
		m_syncSession->RequestStop();
	}
	m_commandManager->Pause();
	ShutdownConnection();

}

void PopSession::PersistFailure(MailError::ErrorCode errCode, const std::string& errMsg)
{
	// update account status with error code and message
	UpdateAccountStatus(m_account, errCode, errMsg);
}

void PopSession::EnableAccount()
{
	MojLogInfo(m_log, "** PopSession::EnableAccount **");

	m_state = State_NeedsConnection;
}

void PopSession::SyncFolder(const MojObject& folderId, bool force)
{
	MojLogInfo(m_log, "PopSession %p: inbox=%s, syncing folder=%s, force=%d", this,
			AsJsonString(m_account->GetInboxFolderId()).c_str(), AsJsonString(folderId).c_str(), force);
	if (folderId != m_account->GetInboxFolderId()) {
		// POP transport only supports inbox sync.
		MojLogInfo(m_log, "PopSession %p will skip non-inbox syncing", this);
		return;
	}

	if (m_state == State_AccountDisabled || m_state == State_AccountDeleted) {
		MojLogInfo(m_log, "Cannot sync an inbox of disabled/deleted account");
		return;
	}

	if (force) {
		// TODO: kills existing inbox sync
		if (HasLoginError() || HasRetryAccountError() || HasNetworkError()) {
			// In State_Connecting means the socket was just reconnected.
			// We don't want to force reconnection again.
			if(m_state != State_Connecting) {
				m_state = State_NeedsConnection;
			}
		}
	} else if (HasLoginError()) {
		MojLogInfo(m_log, "Cannot sync inbox of an account with invalid credentials");
		if (m_syncSession.get()) {
			m_syncSession->RequestStop();
		}
		return;
	} else if (m_state == State_SyncingEmails) {
		MojLogDebug(m_log, "Already syncing inbox emails.");
		return;
	}

	if (m_state == State_NeedsConnection) {
		m_commandManager->Pause();
	}

	MojLogInfo(m_log, "Creating SyncEmailsCommand");
	MojRefCountedPtr<SyncEmailsCommand> command(new SyncEmailsCommand(*this, folderId, m_uidMap));
	m_syncSession->RequestStart();
	m_requestManager->RegisterCommand(command);
	m_commandManager->QueueCommand(command);

	CheckQueue();
}

void PopSession::AutoDownloadEmails(const MojObject& folderId)
{
	if (m_state == State_SyncingEmails || m_state == State_OkToSync) {
		m_state = State_OkToSync;

		MojLogDebug(m_log, "Creating command to auto download email bodies for folder '%s'", AsJsonString(folderId).c_str());
		MojRefCountedPtr<AutoDownloadCommand> command(new AutoDownloadCommand(*this, folderId));
		m_commandManager->QueueCommand(command);
		CheckQueue();
	}
}

void PopSession::FetchEmail(const MojObject& emailId, const MojObject& partId, boost::shared_ptr<DownloadListener>& listener, bool autoDownload)
{
	Request::RequestPtr request(new Request(Request::Request_Fetch_Email, emailId, partId));
	request->SetDownloadListener(listener);
	if (!autoDownload) {
		request->SetPriority(Request::Priority_High);
	}

	if (!autoDownload && m_requestManager->HasCommandHandler()) {
		MojLogInfo(m_log, "Adding fetch email request into request handler");
		m_requestManager->AddRequest(request);
	} else {
		FetchEmail(request);
	}

}

void PopSession::FetchEmail(Request::RequestPtr request)
{
	MojLogInfo(m_log, "Fetching email '%s' with '%s' priority", AsJsonString(request->GetEmailId()).c_str(), ((request->GetPriority() == Request::Priority_High) ? "high" : "low"));
	MojRefCountedPtr<FetchEmailCommand> command(new FetchEmailCommand(*this, request));
	m_commandManager->QueueCommand(command, (request->GetPriority() == Request::Priority_High));
	CheckQueue();
}

void PopSession::DisableAccount(MojServiceMessage* msg)
{
	m_state = State_AccountDisabled;
	CheckQueue();
}

void PopSession::DeleteAccount(MojServiceMessage* msg)
{
}

void PopSession::UpdateAccountStatus(PopAccountPtr account, MailError::ErrorCode errCode, const std::string& errMsg)
{
	MojRefCountedPtr<UpdateAccountStatusCommand> command(new UpdateAccountStatusCommand(*this, account, errCode, errMsg));
	m_commandManager->RunCommand(command);
}


void PopSession::CommandComplete(Command* command)
{
	m_requestManager->UnregisterCommand(command);
	m_commandManager->CommandComplete(command);

	if ((m_state == State_OkToSync || m_state == State_CancelPendingCommands)
			&& m_commandManager->GetPendingCommandCount() == 0
			&& m_commandManager->GetActiveCommandCount() == 0) {
		// no commands to run

		// complete sync session
		if (m_syncSession.get() && (m_syncSession->IsActive() || m_syncSession->IsStarting())) {
			MojLogInfo(m_log, "Requesting to stop sync session");

			if (m_accountError.errorCode != MailError::NONE) {
				MojLogError(m_log, "in session, account error: [%d]%s", m_accountError.errorCode, m_accountError.errorText.c_str());
			}
			m_syncSession->SetAccountError(m_accountError);
			m_syncSession->RequestStop();
		}

		// apply pending account changes
		ApplyAccountUpdates();

		// set the state so that we can log out the current session
		m_state = State_LogOut;
		LogOut();
	}
}

void PopSession::CommandFailed(Command* command, MailError::ErrorCode errCode, const std::exception& exc, bool logErrorToAccount)
{
	if (logErrorToAccount) {
		PersistFailure(errCode, exc.what());
	}

	if (errCode == MailError::INTERNAL_ERROR) {
		// TODO: write internal error to RDX report
	}

	CommandComplete(command);
}

void PopSession::CancelCommands()
{
	// TODO: need to cancel actively running command first

	MojLogInfo(m_log, "%i command(s) to be canceling in queue",
			m_commandManager->GetPendingCommandCount());

	while (m_commandManager->GetPendingCommandCount() > 0) {
		CommandManager::CommandPtr command = m_commandManager->Top();
		m_commandManager->Pop();

		if (command.get()) {
			command->Cancel();

			if (m_syncSession.get() && m_syncSession->IsActive()) {
				m_syncSession->CommandCompleted(command.get());
			}
		}
	}

	// complete sync session
	if (m_syncSession.get() && (m_syncSession->IsActive() || m_syncSession->IsStarting())) {
		MojLogInfo(m_log, "Requesting to stop sync session");
		m_syncSession->RequestStop();
	}
}

void PopSession::SetState(State toSet)
{
	m_state = toSet;
}

void PopSession::RunCommandsInQueue()
{
	m_commandManager->RunCommands();
}

void PopSession::CheckQueue()
{
	MojLogDebug(m_log, "PopSession: Checking the queue. Current state: %d", m_state);

	switch(m_state)
	{
		case State_None:
		{
			MojLogCritical(m_log, "unable to run commands, no account available");
			break;
		}
		case State_NeedsConnection:
		{
			MojLogInfo(m_log, "State: NeedsConnection");
			MojRefCountedPtr<ConnectCommand> command(new ConnectCommand(*this));
			m_commandManager->RunCommand(command);
			m_state = State_Connecting;
			m_canShutdown = false;

			break;
		}
		case State_Connecting:
		{
			MojLogInfo(m_log, "State: Connecting");
			break;
		}
		case State_CheckTlsSupport:
		{
			MojLogInfo(m_log, "State: CheckTlsSupport");
			MojRefCountedPtr<CheckTlsCommand> command(new CheckTlsCommand(*this));
			m_commandManager->RunCommand(command);
			m_state = State_PendingTlsSupport;

			break;
		}
		case State_NegotiateTlsConnection:
		{
			MojLogInfo(m_log, "State: NegotiateTlsConnect");
			MojRefCountedPtr<NegotiateTlsCommand> command(new NegotiateTlsCommand(*this));
			m_commandManager->RunCommand(command);
			m_state = State_PendingTlsNegotiation;

			break;
		}
		case State_UsernameRequired:
		{
			MojLogInfo(m_log, "State: UsernameRequired");
			MojRefCountedPtr<UserCommand> command(new UserCommand(*this, m_account->GetUsername()));
			m_commandManager->RunCommand(command);
			m_state = State_PendingLogin;

			break;
		}
		case State_PasswordRequired:
		{
			MojLogInfo(m_log, "State: PasswordRequired");
			MojRefCountedPtr<PasswordCommand> command(new PasswordCommand(*this, m_account->GetPassword()));
			m_commandManager->RunCommand(command);
			m_state = State_PendingLogin;

			break;
		}
		case State_PendingLogin:
		{
			break;
		}
		case State_NeedUidMap:
		{
			MojLogInfo(m_log, "State: NeedUidMap");
			MojRefCountedPtr<CreateUidMapCommand> command(new CreateUidMapCommand(*this, m_uidMap));
			m_commandManager->RunCommand(command);
			m_state = State_GettingUidMap;

			break;
		}
		case State_GettingUidMap:
		{
			MojLogInfo(m_log, "State: GettingUidMap");
			break;
		}
		case State_OkToSync:
		{
			MojLogInfo(m_log, "State: OkToSync");
			MojLogInfo(m_log, "%i command(s) to run in queue", m_commandManager->GetPendingCommandCount());
			m_commandManager->Resume();
			RunCommandsInQueue();

			break;
		}
		case State_CancelPendingCommands:
		{
			MojLogInfo(m_log, "State: CancelPendingCommands");
			CancelCommands();

			break;
		}
		case State_InvalidCredentials:
		{
			break;
		}
		case State_AccountDisabled:
		{
			MojLogInfo(m_log, "State: AccountDisabled");
			CancelCommands();

			break;
		}
		case State_AccountDeleted:
		{
			break;
		}
		case State_LogOut:
		{
			MojLogInfo(m_log, "State: LogOut");
			LogOut();

			break;
		}
		case State_LoggingOut:
		{
			break;
		}
		default:
		{

		}
	}
}

void PopSession::LogOut()
{
	m_state = State_LoggingOut;
	MojRefCountedPtr<LogoutCommand> command(new LogoutCommand(*this));
	m_commandManager->RunCommand(command);
}

void PopSession::PendAccountUpdates(PopAccountPtr updatedAccnt)
{
	m_pendingAccount = updatedAccnt;
}

void PopSession::ApplyAccountUpdates()
{
	if (m_pendingAccount.get()) {
		m_account = m_pendingAccount;
		m_pendingAccount.reset();

		// check account's error state
		// TODO: need to figure out whether this will overwrite the runtime error state
		m_accountError.errorCode = m_account->GetError().errorCode;
		m_accountError.errorText = m_account->GetError().errorText;
	}
}

void PopSession::Validate()
{
	CheckQueue();
}

void PopSession::Status(MojObject& status) const
{
	MojErr err;

	err = status.put("state", m_state);
	ErrorToException(err);

	if(m_commandManager->GetActiveCommandCount() > 0 || m_commandManager->GetPendingCommandCount() > 0) {
		MojObject cmStatus;
		m_commandManager->Status(cmStatus);

		err = status.put("commandManager", cmStatus);
		ErrorToException(err);
	}

	if(m_connection.get()) {
		MojObject connectionStatus;
		m_connection->Status(connectionStatus);

		err = status.put("connection", connectionStatus);
		ErrorToException(err);
	}
}
