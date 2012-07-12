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

#ifndef POPSESSION_H_
#define POPSESSION_H_

#include <boost/lexical_cast.hpp>
#include <string>

#include "activity/ActivityBuilderFactory.h"
#include "client/Command.h"
#include "client/CommandManager.h"
#include "client/DownloadListener.h"
#include "client/FileCacheClient.h"
#include "client/PowerManager.h"
#include "client/SyncSession.h"
#include "core/MojRefCount.h"
#include "data/DatabaseInterface.h"
#include "data/PopAccount.h"
#include "data/PopFolder.h"
#include "data/UidMap.h"
#include "network/SocketConnection.h"
#include "request/Request.h"
#include "request/RequestManager.h"
#include "stream/LineReader.h"

class PopAccount;
class SyncSession;
class PopClient;

class PopSession: public MojRefCounted, public Command::Listener
{
public:
	typedef boost::shared_ptr<PopAccount> PopAccountPtr;

	PopSession();
	PopSession(PopAccountPtr account);

	typedef enum {
		State_None,
		State_NeedsConnection,
		State_Connecting,
		State_CheckTlsSupport,
		State_PendingTlsSupport,
		State_NegotiateTlsConnection,
		State_PendingTlsNegotiation,
		State_UsernameRequired,
		State_PasswordRequired,
		State_PendingLogin,
		State_NeedUidMap,
		State_GettingUidMap,
		State_OkToSync,
		State_CancelPendingCommands,
		State_SyncingEmails,
		State_LogOut,
		State_LoggingOut,
		State_InvalidCredentials,
		State_AccountDisabled,
		State_AccountDeleted
	} State;

	// Setter functions
	void SetAccount(PopAccountPtr account);
	void SetClient(PopClient& client);
	void SetConnection(MojRefCountedPtr<SocketConnection> connection);
	void SetDatabaseInterface(boost::shared_ptr<DatabaseInterface> dbInterface);
	void SetPowerManager(MojRefCountedPtr<PowerManager> powerManager);
	void SetSyncSession(MojRefCountedPtr<SyncSession> syncSession);
	void SetFileCacheClient(boost::shared_ptr<FileCacheClient> fileCacheClient) { m_fileCacheClient = fileCacheClient;}
	void SetError(MailError::ErrorCode errCode, const std::string& errMsg);
	void ResetError();

	// Getter functions
	const PopAccountPtr& 				GetAccount() const;
	DatabaseInterface& 					GetDatabaseInterface();
	MojRefCountedPtr<PowerManager>		GetPowerManager();
	MojRefCountedPtr<SyncSession>		GetSyncSession();
	boost::shared_ptr<FileCacheClient> 	GetFileCacheClient();
	MojRefCountedPtr<SocketConnection> 	GetConnection();
	InputStreamPtr&						GetInputStream();
	OutputStreamPtr&					GetOutputStream();
	LineReaderPtr& 						GetLineReader();
	const boost::shared_ptr<UidMap>&	GetUidMap() const;
	MojLogger&							GetLogger() { return m_log; }
	bool								HasLoginError();
	bool								HasNetworkError();
	bool								HasRetryNetworkError();
	bool								HasSSLNetworkError();
	bool								HasRetryAccountError();
	bool								HasRetryError();
	bool								IsSessionShutdown();

	// functions for Pop session commands to call back
	void 		 Connected();
	virtual void ConnectFailure(MailError::ErrorCode errCode, const std::string& errMsg);
	void 		 TlsSupported();
	void 		 TlsNegotiated();
	void 		 TlsFailed();
	void 		 UserOk();
	virtual void LoginSuccess();
	virtual void LoginFailure(MailError::ErrorCode errorCode, const std::string& errorMsg);
	void 		 GotUidMap();
	void 		 SyncCompleted();
	virtual void LogoutDone();
	virtual void Disconnected();
	void 		 ShutdownConnection();
	void  		 Reconnect();

	virtual void PersistFailure(MailError::ErrorCode errCode, const std::string& errMsg);
	void 		 UpdateAccountStatus(PopAccountPtr account, MailError::ErrorCode errCode, const std::string& errMsg);

	// functions for PopClient to invoke
	void EnableAccount();
	void SyncFolder(const MojObject& folderId, bool force = false);
	void AutoDownloadEmails(const MojObject& folderId);
	void FetchEmail(const MojObject& emailId, const MojObject& partId, boost::shared_ptr<DownloadListener>& listener, bool autoDownload = false);
	void FetchEmail(Request::RequestPtr request);
	void DisableAccount(MojServiceMessage* msg);
	void DeleteAccount(MojServiceMessage* msg);

	virtual void CommandComplete(Command* command);
	void		 CommandFailed(Command* command, MailError::ErrorCode errCode, const std::exception& exc, bool logErrorToAccount = false);
	void 		 CancelCommands();

	void Validate();

	void Status(MojObject& status) const;

protected:
	virtual ~PopSession();

	void SetState(State toSet);
	void RunCommandsInQueue();
	void CheckQueue();
	void LogOut();
	void PendAccountUpdates(PopAccountPtr updatedAccnt);
	void ApplyAccountUpdates();

	static MojLogger s_log;

	MojLogger&		m_log;

	MojRefCountedPtr<CommandManager>		m_commandManager;
	MojRefCountedPtr<SocketConnection>		m_connection;
	PopAccountPtr							m_account;
	PopAccountPtr							m_pendingAccount;  // a new updated account that will be set to the session when there is no command is running
	EmailAccount::AccountError				m_accountError;
	boost::shared_ptr<DatabaseInterface>	m_dbInterface;
	boost::shared_ptr<UidMap>				m_uidMap;
	PopClient*								m_client;
	boost::shared_ptr<RequestManager>		m_requestManager;
	MojRefCountedPtr<PowerManager>			m_powerManager;
	MojRefCountedPtr<SyncSession>			m_syncSession;
	MojRefCountedPtr<ActivityBuilderFactory> m_builderFactory;

	bool 									m_reconnect;
	bool									m_canShutdown;
	State									m_state;
	InputStreamPtr							m_inputStream;
	OutputStreamPtr							m_outputStream;
	LineReaderPtr   						m_lineReader;
	boost::shared_ptr<FileCacheClient>		m_fileCacheClient;

	friend class SyncEmailsCommand;
};

#endif /* POPSESSION_H_ */
