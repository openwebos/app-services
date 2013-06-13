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

#include <boost/lexical_cast.hpp>

#include "SmtpBusDispatcher.h"
#include "client/SmtpSession.h"
#include "client/CommandManager.h"
#include "data/SmtpAccount.h"
#include "commands/AccountFinderCommand.h"
#include "commands/ConnectCommand.h"
#include "commands/StartTlsCommand.h"
#include "commands/ExtendedHelloCommand.h"
#include "commands/OldHelloCommand.h"
#include "commands/AuthPlainCommand.h"
#include "commands/AuthYahooCommand.h"
#include "commands/AuthLoginCommand.h"
#include "commands/QuitCommand.h"
#include "commands/SmtpSendMailCommand.h"
#include "SmtpCommon.h"
#include "SmtpClient.h"

#include <iostream>

const char * const	SmtpSession::OUR_DOMAIN = "www.palm.com";
const int	SmtpSession::TIMEOUT_CONNECTION			= 2 * 60;
const int	SmtpSession::TIMEOUT_GREETING			= 5 * 60;
const int	SmtpSession::TIMEOUT_MAIL_FROM			= 5 * 60;
const int	SmtpSession::TIMEOUT_RCPT_TO			= 5 * 60;
const int	SmtpSession::TIMEOUT_DATA_INITIATION	= 2 * 60;
const int	SmtpSession::TIMEOUT_DATA_BLOCK			= 3 * 60;
const int	SmtpSession::TIMEOUT_DATA_TERMINATION	= 10 * 60;
const int	SmtpSession::TIMEOUT_GENERAL			= 5 * 60;

SmtpSession::SmtpSession(MojService * service)
: BusClient(service),
  m_log(SmtpBusDispatcher::s_log),
  m_canShutdown(true),
  m_client(NULL),
  m_commandManager(new CommandManager(1, true)),
  m_currentRequestId(0),
  m_state(State_NeedsAccount),
  m_startedTLS(false),
  m_hasSizeExtension(false),
  m_serverMaxSize(0),
  m_hasTLSExtension(false),
  m_hasChunkingExtension(false),
  m_hasPipeliningExtension(false),
  m_hasAuthExtension(false),
  m_hasPlainAuth(false),
  m_hasLoginAuth(false),
  m_hasYahooAuth(false),
  m_resetAccount(false)
{
}

SmtpSession::SmtpSession(boost::shared_ptr<SmtpAccount> account, MojService * service)
: BusClient(service),
  m_log(SmtpBusDispatcher::s_log),
  m_canShutdown(true),
  m_client(NULL),
  m_commandManager(new CommandManager(1, true)),
  m_account(account),
  m_currentRequestId(0),
  m_state(State_Unconnected),
  m_startedTLS(false),
  m_hasSizeExtension(false),
  m_serverMaxSize(0),
  m_hasTLSExtension(false),
  m_hasChunkingExtension(false),
  m_hasPipeliningExtension(false),
  m_hasAuthExtension(false),
  m_hasPlainAuth(false),
  m_hasLoginAuth(false),
  m_hasYahooAuth(false),
  m_resetAccount(false)
{
}

SmtpSession::~SmtpSession()
{
}

const char* SmtpSession::GetStateName(State state)
{
	switch(state) {
	case State_NeedsAccount:				return "State_NeedsAccount";
	case State_LoadingAccount:				return "State_LoadingAccount";
	case State_Unconnected:					return "State_Unconnected";
	case State_NeedsConnection:				return "State_NeedsConnection";
	case State_Connecting:					return "State_Connecting";
	case State_SendExtendedHelloCommand:	return "State_SendExtendedHelloCommand";
	case State_SendingExtendedHelloCommand:	return "State_SendingExtendedHelloCommand";
	case State_SendOldHelloCommand:			return "State_SendOldHelloCommand";
	case State_SendingOldHelloCommand:		return "State_SendingOldHelloCommand";
	case State_SendStartTlsCommand:			return "State_SendStartTlsCommand";
	case State_SendingStartTlsCommand:		return "State_SendingStartTlsCommand";
	case State_SendAuthPlainCommand:		return "State_SendAuthPlainCommand";
	case State_SendingAuthPlainCommand:		return "State_SendingAuthPlainCommand";
	case State_SendAuthYahooCommand:		return "State_SendAuthYahooCommand";
	case State_SendingAuthYahooCommand:		return "State_SendingAuthYahooCommand";
	case State_SendAuthLoginCommand:		return "State_SendAuthLoginCommand";
	case State_SendingAuthLoginCommand:		return "State_SendingAuthLoginCommand";
	case State_LoginSuccess:				return "State_LoginSuccess";
	case State_NeedsCancelling:				return "State_NeedsCancelling";
	case State_LoggedIn:					return "State_LoggedIn";
	case State_InvalidCredentials:			return "State_InvalidCredentials";
	case State_AccountDeleted:				return "State_AccountDeleted";
	case State_SendQuitCommand:				return "State_SendQuitCommand";
	case State_SendingQuitCommand:			return "State_SendingQuitCommand";
	}

	return "unknown";
}

MojRefCountedPtr<MojServiceRequest> SmtpSession::CreateRequest()
{
	MojLogTrace(m_log);

	assert(m_service); 
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);
	ErrorToException(err);
	return req;
}

void SmtpSession::AccountFinderSuccess(boost::shared_ptr<SmtpAccount> account)
{
	if (m_state == State_LoadingAccount) {
		MojLogInfo(m_log, "AccountFinderSuccess received new account");
		m_account = account;
		RunState(State_Unconnected);
	} else {
		MojLogInfo(m_log, "AccountFinderSuccess triggered in wrong state %d.", m_state);
		InternalFailure("AccountFinderSuccess triggered in wrong state.");
	}
}

void SmtpSession::AccountFinderFailure(SmtpError error)
{
	// FIXME: is this right? Not sure how likely we are to get here.
	Failure(error); 
}

void SmtpSession::ConnectSuccess()
{
	if (m_state == State_Connecting) {
		m_startedTLS = false;
		RunState(State_SendExtendedHelloCommand);
	} else {
		MojLogInfo(m_log, "ConnectSuccess triggered in wrong state %d.", m_state);
		InternalFailure("ConnectSuccess triggered in wrong state.");
	}
}

void SmtpSession::ConnectFailure(SmtpError error)
{
	MojLogError(m_log, "ConnectFailure.");
	Disconnect();
	Failure(error);
}

void SmtpSession::HelloSuccess()
{
	if (m_state == State_SendingExtendedHelloCommand) {
		if (m_hasTLSExtension && m_account->WantsTLS()) {
			m_startedTLS = true;
			RunState(State_SendStartTlsCommand);
		} else {
			if (!m_startedTLS && m_account->NeedsTLS()) {
				SmtpError error;
				error.errorCode = MailError::CONFIG_NO_SSL;
				error.errorText = "";
				error.errorOnAccount = true;
				error.internalError = "TLS required by user, but not advertised by EHLO.";
				Failure(error);
				return;
			}
			if (m_hasYahooAuth && m_account->GetYahooToken().length() > 0 ) {
				// Yahoo server, and we loaded a yahoo security token during account loading
				RunState(State_SendAuthYahooCommand);
			} else if (m_hasLoginAuth && m_account->UseSmtpAuth()) {
				RunState(State_SendAuthLoginCommand);
			} else if (m_hasPlainAuth && m_account->UseSmtpAuth()) {
				RunState(State_SendAuthPlainCommand);
			} else {
				// NOTE: no auth needed?
				RunState(State_LoginSuccess);
				// N.B. Watch for 5xx errors on subsequent commands indicating need for credentials
			}
		}
	} else if (m_state == State_SendingOldHelloCommand) {
		if (!m_startedTLS && m_account->NeedsTLS()) {
			SmtpError error;
			error.errorCode = MailError::ACCOUNT_UNKNOWN_AUTH_ERROR;
			error.errorText = "";
			error.errorOnAccount = true;
			error.internalError = "TLS required, but not advertised by HELO.";
			Failure(error);
			return;
		}
		RunState(State_LoginSuccess);
	} else {
		MojLogInfo(m_log, "HelloSuccess triggered in wrong state.");
		InternalFailure("HelloSuccess triggered in wrong state.");
	}
}

void SmtpSession::HelloFailure(SmtpError code)
{
	if (m_state == State_SendingExtendedHelloCommand) {
		// ignore code, just try old hello
		MojLogInfo(m_log, "Extended hello not supported, falling back to old hello.");
		RunState(State_SendOldHelloCommand);
	} else {
		MojLogError(m_log, "HelloFailure in wrong state.");
		InternalFailure("HelloFailure triggered in wrong state.");
	}
}

void SmtpSession::Failure(SmtpError error) {
	MojLogError(m_log, "Failure in SmtpSession %p: code=%d, text=%s, internalText=%s, account=%d, email=%d",
		this,
		error.errorCode,
		error.errorText.c_str(),
		error.internalError.c_str(),
		error.errorOnAccount,
		error.errorOnEmail);
	m_error = error;
	RunState(State_NeedsCancelling);
}

void SmtpSession::InternalFailure(std::string internalText)
{
	SmtpError error;
	error.errorCode = MailError::INTERNAL_ERROR;
	error.errorText = "";
	error.errorOnAccount = true;
	error.internalError = internalText;
	Failure(error);
}

bool SmtpSession::HasError()
{
	return m_error.errorCode != 0;
}

SmtpSession::SmtpError SmtpSession::GetError()
{
	return m_error;
}

void SmtpSession::ClearError()
{
	MojLogInfo(m_log, "Resetting error.");
	m_error.clear();
}

void SmtpSession::TlsSuccess()
{
	if (m_state == State_SendingStartTlsCommand) {
		// Restart the extension negotiation
		RunState(State_SendExtendedHelloCommand);
	} else {
		MojLogInfo(m_log, "TlsSuccess triggered in wrong state.");
		InternalFailure("TlsSuccess triggered in wrong state.");
	}

}

void SmtpSession::TlsFailure(SmtpError error)
{
	MojLogError(m_log, "TlsFailure.");
	Failure(error);
}

void SmtpSession::AuthYahooSuccess()
{
	if (m_state == State_SendingAuthYahooCommand) {
		RunState(State_LoginSuccess);
	} else {
		MojLogInfo(m_log, "AuthYahooSuccess triggered in wrong state.");
		InternalFailure("AuthYahooSuccess triggered in wrong state.");
	}

}

void SmtpSession::AuthYahooFailure(SmtpError error)
{
	MojLogError(m_log, "AuthYahooFailure.");
	Failure(error);
}

void SmtpSession::AuthPlainSuccess()
{
	if (m_state == State_SendingAuthPlainCommand) {
		RunState(State_LoginSuccess);
	} else {
		MojLogInfo(m_log, "AuthPlainSuccess triggered in wrong state.");
		InternalFailure("AuthPlainSuccess triggered in wrong state.");
	}

}

void SmtpSession::AuthPlainFailure(SmtpError error)
{
	MojLogError(m_log, "AuthPlainFailure.");
	Failure(error);
}

void SmtpSession::AuthLoginSuccess()
{
	if (m_state == State_SendingAuthLoginCommand) {
		RunState(State_LoginSuccess);
	} else {
		MojLogInfo(m_log, "AuthLoginSuccess triggered in wrong state.");
		InternalFailure("AuthLoginSuccess triggered in wrong state.");
	}

}

void SmtpSession::AuthLoginFailure(SmtpError error)
{
	MojLogError(m_log, "AuthLoginFailure.");
	Failure(error);
}

void SmtpSession::QuitSuccess()
{
	if (m_state == State_SendingQuitCommand) {
		Disconnect();
		ClearError();
		if (m_resetAccount)
			RunState(State_NeedsAccount);
		else
			RunState(State_Unconnected);
	} else {
		MojLogInfo(m_log, "QuitSuccess triggered in wrong state.");
		InternalFailure("QuitSuccess triggered in wrong state.");
	}
}

void SmtpSession::QuitFailure(SmtpError error)
{
	//ignore error code
	
	MojLogError(m_log, "QuitFailure.");
	
	Disconnect();

	ClearError();

	RunState(State_Unconnected);
}

void SmtpSession::SetAccountId(MojObject& accountId)
{
	MojLogTrace(m_log);

	m_accountId = accountId;
}

void SmtpSession::SetConnection(MojRefCountedPtr<SocketConnection> connection)
{
	m_connection = connection;

	if(connection.get()) {
		m_inputStream = connection->GetInputStream();
		m_outputStream = connection->GetOutputStream();
	}
}

void SmtpSession::SetDatabaseInterface(boost::shared_ptr<DatabaseInterface> dbInterface)
{
	m_dbInterface = dbInterface;
}

void SmtpSession::SetFileCacheClient(boost::shared_ptr<FileCacheClient> fileCacheClient)
{
	m_fileCacheClient = fileCacheClient;
}

void SmtpSession::SetState(State toSet)
{
	MojLogDebug(m_log, "changing session %p state from %d to %d", this, m_state, toSet);
	m_state = toSet;
	
	if (toSet != State_LoggedIn) {
		// Disable running commands until we are logged in
		m_commandManager->Pause();
	}
}

void SmtpSession::RunState(State newState)
{
	SetState(newState);
	CheckQueue();
}

DatabaseInterface& SmtpSession::GetDatabaseInterface()
{
	return *m_dbInterface.get();
}

FileCacheClient& SmtpSession::GetFileCacheClient()
{
	return *m_fileCacheClient.get();
}

const boost::shared_ptr<SmtpAccount>& SmtpSession::GetAccount()
{
	return m_account;
}

LineReaderPtr& SmtpSession::GetLineReader()
{
	if(m_lineReader.get() == NULL)
		m_lineReader.reset(new LineReader(GetInputStream()));

	return m_lineReader;
}

InputStreamPtr& SmtpSession::GetInputStream()
{
	assert( m_inputStream.get() != NULL );
	return m_inputStream;
}

OutputStreamPtr& SmtpSession::GetOutputStream()
{
	assert( m_outputStream.get() != NULL );
	return m_outputStream;
}

MojRefCountedPtr<SocketConnection> SmtpSession::GetConnection()
{
	return m_connection;
}

void SmtpSession::Disconnect()
{
	MojLogInfo(m_log, "session %p disconnecting from server", this);
        
	if(m_connection.get()) {
		m_connection->Shutdown();
	}

	m_connection.reset();
	m_inputStream.reset();
	m_outputStream.reset();
	m_lineReader.reset();
	
	Disconnected();
}

void SmtpSession::Disconnected()
{
	MojLogInfo(m_log, "Session %p disconnected", this);

	m_canShutdown = true;

	if(m_client) {
		m_client->SessionShutdown();
	}
}                                                                                        

void SmtpSession::CommandComplete(Command* command)
{
	m_commandManager->CommandComplete(command);
	
	if (m_resetAccount) {
		RunState(State_SendQuitCommand);
	}

	// Disconnect from the server after running all commands.
	if(m_state == State_NeedsCancelling && m_commandManager->GetPendingCommandCount() == 0
			&& m_commandManager->GetActiveCommandCount() == 0) {
		MojLogInfo(m_log, "no commands active or pending after cancelling, quitting and resetting");

		// Go to the state for a clean disconnect  
		RunState(State_SendQuitCommand);
		return;
	}

	// Disconnect from the server after running all commands.
	if(m_state == State_LoggedIn && m_commandManager->GetPendingCommandCount() == 0
			&& m_commandManager->GetActiveCommandCount() == 0) {
		MojLogInfo(m_log, "no commands active or pending, quitting");

		// Go to the state for a clean disconnect  
		RunState(State_SendQuitCommand);
		return;
	}
}

void SmtpSession::CheckQueue()
{
	MojLogTrace(m_log);
	MojLogInfo(m_log, "checking the session queue. current state: %d. commands active: %d pending: %d\n",
					m_state, m_commandManager->GetActiveCommandCount(), m_commandManager->GetPendingCommandCount());

	assert( m_state == State_NeedsAccount || m_account.get() || m_state == State_NeedsCancelling);
	
	// If we need to reset the account, put the queue on hold -- it'll wake itself back up after
	// the account is reloaded.
	if (m_resetAccount && m_state == State_LoggedIn) {
		m_state = State_SendQuitCommand;
		m_commandManager->Pause();
	}
	
	// If there is anything on the queue and we are not connected, bring up the connection.
	if (m_commandManager->GetPendingCommandCount() > 0 && m_state == State_Unconnected) {
		if (HasError()) {
			MojLogInfo(m_log, "actions in queue, but account has error, so setting state to NeedsCancelling");
			m_state = State_NeedsCancelling;
		} else {
			MojLogInfo(m_log, "actions in queue, setting state to NeedsConnection");
			m_state = State_NeedsConnection;
		}
		// The queue will remain paused until we are logged in.
	}
	                                  
	switch(m_state)
	{
		case State_NeedsAccount:
		{
			m_canShutdown = false;

			MojLogInfo(m_log, "State of SmtpSession %p: NeedsAccount", this);
			MojLogInfo(m_log, "Loading smtp account");
			m_resetAccount = false; // Only clear this before we start a new AccountFinder,
									// in case an accountUpdate happens in the middle of finding the current account.
			MojRefCountedPtr<AccountFinderCommand> accountFinder(new AccountFinderCommand(*this, m_accountId));
			m_commandManager->RunCommand(accountFinder); 
			
			SetState(State_LoadingAccount);
			break;
		}
		case State_LoadingAccount:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: LoadingAccount", this);
			break;
		}
		case State_Unconnected:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: Unconnected", this);
			break;
		}
		case State_NeedsCancelling:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: NeedsCancelling", this);
			if (m_commandManager->GetPendingCommandCount() == 0
			&& m_commandManager->GetActiveCommandCount() == 0) {
				MojLogInfo(m_log, "no commands active or pending, quitting");
				// Go to the state for a clean disconnect
				RunState(State_SendQuitCommand);
			} else {
				m_commandManager->Resume(); // expect commands will die off with errors due to m_session.HasError()
			}
			break;
		}
		case State_NeedsConnection:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: NeedsConnection", this);
			
			SetState(State_Connecting);
		
			MojRefCountedPtr<ConnectCommand> command(new ConnectCommand(*this));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_Connecting:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: Connecting", this);
			break;
		}
		case State_SendExtendedHelloCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendExtendedHelloCommand", this);
			SetState(State_SendingExtendedHelloCommand);
		
			MojRefCountedPtr<ExtendedHelloCommand> command(new ExtendedHelloCommand(*this, OUR_DOMAIN));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_SendingExtendedHelloCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendingExtendedHelloCommand", this);
			
			break;
		}
		case State_SendOldHelloCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendOldHelloCommand", this);
			SetState(State_SendingOldHelloCommand);
		
			MojRefCountedPtr<OldHelloCommand> command(new OldHelloCommand(*this, OUR_DOMAIN));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_SendingOldHelloCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendingOldHelloCommand", this);
			
			break;
		}
		case State_SendStartTlsCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendStartTlsCommand", this);
			SetState(State_SendingStartTlsCommand);
		
			MojRefCountedPtr<StartTlsCommand> command(new StartTlsCommand(*this));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_SendingStartTlsCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendingStartTlsCommand", this);
			break;
		}
		case State_SendAuthYahooCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendAuthYahooCommand", this);
			SetState(State_SendingAuthYahooCommand);
		
			MojRefCountedPtr<AuthYahooCommand> command(new AuthYahooCommand(*this));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_SendingAuthYahooCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendingAuthYahooCommand", this);
			break;
		}
		case State_SendAuthPlainCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendAuthPlainCommand", this);
			SetState(State_SendingAuthPlainCommand);
		
			MojRefCountedPtr<AuthPlainCommand> command(new AuthPlainCommand(*this));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_SendingAuthPlainCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendingAuthPlainCommand", this);
			break;
		}
		case State_SendAuthLoginCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendAuthLoginCommand", this);
			SetState(State_SendingAuthLoginCommand);
		
			MojRefCountedPtr<AuthLoginCommand> command(new AuthLoginCommand(*this));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_SendingAuthLoginCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendingAuthLoginCommand", this);
			break;
		}
		case State_LoginSuccess:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: LoginSuccess", this);
			LoginSuccess();
			break;
		}
		case State_LoggedIn:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: LoggedIn", this);
			m_commandManager->Resume();
			break;
		}
		case State_SendQuitCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendQuitCommand", this);
			
			if (m_connection.get() && !m_connection->IsClosed()) {
				SetState(State_SendingQuitCommand);
		
				MojRefCountedPtr<QuitCommand> command(new QuitCommand(*this));
				m_commandManager->RunCommand(command);
			} else {
				ClearError();
				if (m_resetAccount)  {
					MojLogInfo(m_log, "Updating account state to NeedsAccount");
					RunState(State_NeedsAccount);
				}
				else {
					MojLogInfo(m_log, "Updating account state to Unconnected");
					RunState(State_Unconnected);
				}
			}
			break;
		}
		case State_SendingQuitCommand:
		{
			MojLogInfo(m_log, "State of SmtpSession %p: SendingQuitCommand", this);
			break;
		}
		default:
		{

		}
	}
}

void SmtpSession::SendMail(const MojObject emailId, MojSignal<SmtpSession::SmtpError>::SlotRef doneSignal)
{
	MojLogTrace(m_log);

	MojRefCountedPtr<SmtpSendMailCommand> send = new SmtpSendMailCommand(*this, emailId);
	send->SetSlots(doneSignal);
	m_commandManager->QueueCommand(send, true);
	
	CheckQueue();
}

void SmtpSession::ClearAccount()
{
	MojLogTrace(m_log);

	MojLogInfo(m_log, "Clearing account state on session %p, in state %d", this, m_state);
	
	m_resetAccount = true;

	// If we're in Unconnected, we will be quiescent, so we need to kick to a different state.
	// Other states will either progress to LoggedIn or Unconnected, and CommandComplete will
	// perform this transition for us.
	if (m_state == State_Unconnected) {
		RunState(State_NeedsAccount);
	}
}

void SmtpSession::RunCommandsInQueue()
{
	m_commandManager->RunCommands();
}

void SmtpSession::Validate()
{
	SetState(State_NeedsConnection);
	CheckQueue();
}

void SmtpSession::Send()
{
	CheckQueue();
}

void SmtpSession::LoginSuccess()
{
	MojLogInfo(m_log, "Login success.");
	RunState(State_LoggedIn);
}

void SmtpSession::SendSuccess()
{
	MojLogError(m_log, "Send success.\n");
	RunState(State_SendQuitCommand);
}

void SmtpSession::SendFailure(const std::string& errorText)
{
	MojLogError(m_log, "Failed to send (%s).\n", errorText.c_str());
	RunState(State_SendQuitCommand);
}

void SmtpSession::HasSizeExtension(bool value)
{
	m_hasSizeExtension = value;
}

void SmtpSession::HasSizeMax(int value)
{
	m_serverMaxSize = value;
}

bool SmtpSession::GetSizeMax()
{
	return m_hasSizeExtension;
}

size_t SmtpSession::GetSizeMaxValue()
{
	if (!m_hasSizeExtension)
		return 0;
	else
		return m_serverMaxSize;
}

void SmtpSession::HasTLSExtension(bool value)
{
	m_hasTLSExtension = value;
}

void SmtpSession::HasChunkingExtension(bool value)
{
	m_hasChunkingExtension = value;
}

void SmtpSession::HasPipeliningExtension(bool value)
{
	m_hasPipeliningExtension = value;
}

void SmtpSession::HasAuthExtension(bool value)
{
	m_hasAuthExtension = value;
}

void SmtpSession::HasPlainAuth(bool value)
{
	m_hasPlainAuth = value;
}

void SmtpSession::HasLoginAuth(bool value)
{
	m_hasLoginAuth = value;
}

void SmtpSession::HasYahooAuth(bool value)
{
	m_hasYahooAuth = value;
}

void SmtpSession::Status(MojObject& status) const
{
	MojErr err;

	err = status.putString("state", GetStateName(m_state));
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

	if(m_lineReader.get()) {
		MojObject lineReaderStatus;
		m_lineReader->Status(lineReaderStatus);

		err = status.put("lineReader", lineReaderStatus);
		ErrorToException(err);
	}

	if(m_account.get()) {
		MojObject accountInfo;

		err = accountInfo.putString("templateId", m_account->GetTemplateId().c_str());
		ErrorToException(err);

		err = status.put("accountInfo", accountInfo);
		ErrorToException(err);
	}
}
