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

#ifndef SMTPSESSION_H_
#define SMTPSESSION_H_

#include <boost/lexical_cast.hpp>
#include <string>

#include "CommonErrors.h"
#include "client/BusClient.h"
#include "client/Command.h"
#include "client/CommandManager.h"
#include "core/MojRefCount.h"
#include "data/DatabaseInterface.h"
#include "data/SmtpAccount.h"
#include "network/SocketConnection.h"
#include "stream/LineReader.h"
#include "client/FileCacheClient.h"
#include "SmtpCommon.h"

class SmtpAccount;
class SmtpClient;

// TODO: session should not be a BusClient.  Move the logic to SmtpClient.
class SmtpSession: public BusClient, public Command::Listener
{
public:
	struct SmtpError {
		MailError::ErrorCode errorCode;	// Standard code
		std::string errorText;			// Server provided text
		std::string internalError;		// Internal text, not localized or shown to user
		bool errorOnAccount;			// If true, error applies to entire account (might be temporary or permanent)
		bool errorOnEmail;				// If true, error applies to specific email (considered permanent)
	
		// determine whether error is of a type that should disallow
		// automatic retries, to avoid tripping security lock-out, or because
		// we otherwise know that we will not be able to continue.
		// FIXME: On web-account-lockout we might want to back off retries, even though
		// we don't entirely lock it out.
		bool IsLockoutError() { return errorCode == MailError::BAD_USERNAME_OR_PASSWORD ||
						errorCode == MailError::INTERNAL_ACCOUNT_MISCONFIGURED; }
		
		// determine whether error is of a type that shouldn't be
		// published to the account, as it is not significant --
		// such as network disconnection or timeout.
		bool IsTrivialError() { return errorCode == MailError::CONNECTION_FAILED ||
										errorCode == MailError::CONNECTION_TIMED_OUT ||
										errorCode == MailError::NO_NETWORK ||
										errorCode == MailError::HOST_NOT_FOUND; }
		
		SmtpError() : errorCode(MailError::NONE),  errorText(""), internalError(""), errorOnAccount(false), errorOnEmail(false) {}

		SmtpError(MailError::ErrorInfo error) {
			errorCode = error.errorCode;
			errorText = error.errorText;
			internalError = "";
			errorOnAccount = true;
			errorOnEmail = false;
		}
		void clear() {
			errorCode = MailError::NONE;
			errorText = "";
			internalError = "";
			errorOnAccount = false;
			errorOnEmail = false;
		}

		void Status(MojObject& status) const {
			MojErr err;
			err = status.put("errorCode", errorCode);
			ErrorToException(err);
			err = status.putString("errorText", errorText.c_str());
			ErrorToException(err);
			err = status.putString("internalError", errorText.c_str());
			ErrorToException(err);

			if(errorOnAccount) {
				err = status.put("errorOnAccount", errorOnAccount);
				ErrorToException(err);
			}

			if(errorOnEmail) {
				err = status.put("errorOnEmail", errorOnEmail);
				ErrorToException(err);
			}
		}
	};

	SmtpSession(MojService * service);
	SmtpSession(boost::shared_ptr<SmtpAccount> account, MojService * service);
	virtual ~SmtpSession();

	// Remember to update GetStateName()
	enum State {
		State_NeedsAccount,
		State_LoadingAccount,
		
		State_Unconnected,
		
		State_NeedsConnection,
		State_Connecting,
		
		State_SendExtendedHelloCommand,
		State_SendingExtendedHelloCommand,
		
		State_SendOldHelloCommand,
		State_SendingOldHelloCommand,
		
		State_SendStartTlsCommand,
		State_SendingStartTlsCommand,
		
		State_SendAuthPlainCommand,
		State_SendingAuthPlainCommand,
		
		State_SendAuthYahooCommand,
		State_SendingAuthYahooCommand,
		
		State_SendAuthLoginCommand,
		State_SendingAuthLoginCommand,
		
		State_LoginSuccess,
		
		State_NeedsCancelling,
		
		State_LoggedIn,
		State_InvalidCredentials,
		State_AccountDeleted,
		
		State_SendQuitCommand,
		State_SendingQuitCommand
	};

	static const char* GetStateName(State state);

	void AccountFinderSuccess(boost::shared_ptr<SmtpAccount> account);
	void AccountFinderFailure(SmtpError erro);
	void ConnectSuccess();
	void ConnectFailure(SmtpError error);
	void TlsFailure(SmtpError error);
	void HelloSuccess();
	void HelloFailure(SmtpError error);
	void AuthPlainSuccess();
	void AuthPlainFailure(SmtpError error);
	void AuthYahooSuccess();
	void AuthYahooFailure(SmtpError error);
	void AuthLoginSuccess();
	void AuthLoginFailure(SmtpError error);
	void QuitSuccess();
	void QuitFailure(SmtpError error);
	
	bool HasError();
	SmtpError GetError();
	void ClearError();
	
	virtual void Failure(SmtpError error);
	void InternalFailure(std::string);

	virtual void LoginSuccess();
	virtual void TlsSuccess();
	
	virtual void SendSuccess();
	virtual void SendFailure(const std::string& errorText);
	
	virtual void Disconnected();
	
	void HasSizeExtension(bool);
	void HasSizeMax(int);
	void HasTLSExtension(bool);
	void HasPipeliningExtension(bool);
	void HasChunkingExtension(bool);
	void HasAuthExtension(bool);
	void HasPlainAuth(bool);
	void HasLoginAuth(bool);
	void HasYahooAuth(bool);
	
	bool GetSizeMax();
	size_t GetSizeMaxValue();
	                                                                                                                
	void SetAccountId(MojObject& accountId);
	void SetAccountErrorCode(MailError::ErrorCode);
	MailError::ErrorCode GetAccountErrorCode();
	void SetConnection(MojRefCountedPtr<SocketConnection> connection);
	void SetDatabaseInterface(boost::shared_ptr<DatabaseInterface> dbInterface);
	void SetFileCacheClient(boost::shared_ptr<FileCacheClient> fileCacheClient);

	MojRefCountedPtr<SocketConnection> 	GetConnection();
	InputStreamPtr&						GetInputStream();
	OutputStreamPtr&					GetOutputStream();
	LineReaderPtr& 						GetLineReader();
	MojLogger&							GetLogger() { return m_log; }
	DatabaseInterface& 					GetDatabaseInterface();
	FileCacheClient& 					GetFileCacheClient();
	const boost::shared_ptr<SmtpAccount>& GetAccount();

	virtual void CommandComplete(Command* command);

	void Validate();
	void Send();
	
	void Status(MojObject& status) const;
	void Disconnect();
	
	void SaveEmail(MojObject email, MojObject accountId, bool isDraft);
	void SendMail(const MojObject emailId, MojSignal<SmtpSession::SmtpError>::SlotRef);
	void ClearAccount();
	
	MojRefCountedPtr<MojServiceRequest> CreateRequest();

	void SetClient(SmtpClient* client) { m_client = client; }
	bool IsReadyForShutdown() { return m_canShutdown; }

	static const int	TIMEOUT_CONNECTION;
	static const int	TIMEOUT_GREETING;
	static const int	TIMEOUT_MAIL_FROM;
	static const int	TIMEOUT_RCPT_TO;
	static const int	TIMEOUT_DATA_INITIATION;
	static const int	TIMEOUT_DATA_BLOCK;
	static const int	TIMEOUT_DATA_TERMINATION;
	static const int	TIMEOUT_GENERAL;

protected:

	static const char * const OUR_DOMAIN;

	void SetState(State toSet);
	void RunState(State toSet);
	

	void RunCommandsInQueue();
	void CheckQueue();

	MojLogger&		m_log;

	bool									m_canShutdown;
	SmtpClient*								m_client;
	MojRefCountedPtr<CommandManager>		m_commandManager;
	MojRefCountedPtr<SocketConnection>		m_connection;
	boost::shared_ptr<SmtpAccount>			m_account;
	MojObject								m_accountId;
	boost::shared_ptr<DatabaseInterface>	m_dbInterface;
	boost::shared_ptr<FileCacheClient>		m_fileCacheClient;
	SmtpError								m_error;

	int				m_currentRequestId;
	State			m_state;
	InputStreamPtr	m_inputStream;
	OutputStreamPtr	m_outputStream;
	LineReaderPtr   m_lineReader;
	bool			m_startedTLS;
	
	bool			m_hasSizeExtension;
	int				m_serverMaxSize;
	bool			m_hasTLSExtension;
	bool			m_hasChunkingExtension;
	bool			m_hasPipeliningExtension;
	bool			m_hasAuthExtension;
	bool			m_hasPlainAuth;
	bool			m_hasLoginAuth;
	bool			m_hasYahooAuth;
	
	bool			m_resetAccount;
	
};
#endif /* SMTPSESSION_H_ */
