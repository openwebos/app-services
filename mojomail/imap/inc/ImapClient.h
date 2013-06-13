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

#ifndef IMAPCLIENT_H_
#define IMAPCLIENT_H_

#include "activity/Activity.h"
#include "client/CommandManager.h"
#include "commands/ImapCommand.h"
#include "client/ImapRequestManager.h"
#include "client/BusClient.h"
#include "core/MojServiceMessage.h"
#include "activity/Activity.h"
#include <map>
#include "exceptions/MailException.h"
#include "client/SyncParams.h"
#include "core/MojServiceMessage.h"
#include "core/MojSignal.h"
#include "commands/UpdateAccountCommand.h"
#include "commands/CreateDefaultFoldersCommand.h"
#include "activity/NetworkStatus.h"

/*
 * \brief
 * The ImapClient is a sync state machine.  It manages the lifecycle of all ImapCommand
 * objects and determines when they will be executed.
 */
class ImapSession;
class DatabaseInterface;
class MojLunaService;
class ImapAccount;
class FileCacheClient;
class DownloadListener;
class SearchRequest;
class SyncSession;
class ImapSyncSessionCommand;
class PowerManager;
class ImapBusDispatcher;
class NetworkStatusMonitor;

class ImapClient : public MojSignalHandler, public BusClient, public Command::Listener
{
	typedef MojRefCountedPtr<ImapSession> ImapSessionPtr;
	typedef MojRefCountedPtr<SyncSession> SyncSessionPtr;
public:

	ImapClient(MojLunaService* service, ImapBusDispatcher* dispatcher, const MojObject& accountId, const boost::shared_ptr<DatabaseInterface>& dbInterface);
	virtual ~ImapClient();

	// singleton session // possibly in an array // just for now
	// pick the first session and call the corresponding method
	// move session from busDispatcher to client
	// bus dispatcher should only call this to do stuff
	// might call validator directly from dispatcher

	void SetAccount(const boost::shared_ptr<ImapAccount>& account);

	/**
	 * Sync all folders/collections in the account with autoSync enabled.
	 */
	void SyncAccount(SyncParams params);

	/**
	 * Disabling the account.
	 */
	void DisableAccount(MojServiceMessage* msg, MojObject& payload);

	/**
	 * Deleting the account.
	 */
	void DeleteAccount(MojServiceMessage* msg, MojObject& payload);

	/**
	 * Syncs a given e-mail folder
	 * @param id the ID of the com.palm.folder record
	 * @param force
	 */
	void SyncFolder(const MojObject& folderId, SyncParams syncParams);

	void StartFolderSync(const MojObject& folderId, SyncParams syncParams);

	void CheckOutbox(SyncParams params);

	void CheckDrafts(const ActivityPtr& activity);

	void UploadSentEmails(const ActivityPtr& activity);

	void UploadDrafts(const ActivityPtr& activity);

	/**
	 * Download a message part.
	 * @param folderId
	 * @param emailId
	 * @param partId may be undefined/null to download the first body part
	 */
	void DownloadPart(const MojObject& folderId, const MojObject& emailId, const MojObject& partId, const MojRefCountedPtr<DownloadListener>& listener);

	/**
	 * Download a message part.
	 * @param folderId
	 * @param searchRequest
	 */
	void SearchFolder(const MojObject& folderId, const MojRefCountedPtr<SearchRequest>& searchRequest);


	/**
	 * Downloads the message body for the given e-mail
	 */
//	void FetchEmail(const MojObject& emailId);
	// create new fetch command and tell session to run

	// probably have a state machine
	// states for command executing
	// - needs acct info, get acct info from database. assumes you already have an account
	//        constructor takes an acct id. Syncs account immediately (post to queue first)
	//           flesh this out with jason
	//
 	// - create session
	// - login failed
	// - run_command, safe to run commands

	/**
	 * Handles completion of a command
	 */
	void CommandComplete(Command* command);

	/**
	 * Handles failure of a command
	 */
	void CommandFailure(ImapCommand* command);
	/**
	 * Feeder function for command failure
	 */
	void CommandFailure(ImapCommand* command, const std::exception& e);

	void LoginSuccess();

	// Callback when a session is disconnected
	// FIXME use session pointer not folderId
	void SessionDisconnected(MojObject& folderId);

	void UpdateAccount(const MojObject& accountId, const ActivityPtr& activity, bool credentialsChanged);

	// Callback after account is deleted
	void DeletedAccount();

	// Callback after account is disabled
	void DisabledAccount();

	void EnableAccount(MojServiceMessage* msg);

	void Status(MojObject& status);

	bool DisableAccountInProgress();

	void WakeupIdle(const MojObject& folderId);

	DatabaseInterface& GetDatabaseInterface() const { return *m_dbInterface.get(); }

	FileCacheClient& GetFileCacheClient() { return *m_fileCacheClient.get(); }

	MojLogger& GetLogger() { return m_log; }

	const MojObject& GetAccountId() const { return m_accountId; }

	ImapAccount& GetAccount() const
	{
		if(m_account.get())
			return *m_account.get();
		else
			throw MailException("no account available", __FILE__, __LINE__);
	}

	/**
	 * Start a sync session
	 */
	void StartSyncSession(const MojObject& folderId, const std::vector<ActivityPtr>& activities);

	/**
	 * Adds a command to the sync session for a folder, creating and starting up
	 * a new sync session if necessary.
	 */
	void AddToSyncSession(const MojObject& folderId, ImapSyncSessionCommand* command);

	/**
	 * Adds a command to the sync session for a folder, creating and starting up
	 * a new sync session if necessary. The command will resume running when the
	 * session is active.
	 */
	virtual void RequestSyncSession(const MojObject& folderId, ImapSyncSessionCommand* command, MojSignal<>::SlotRef slot);

	void UpdateAccountStatus(const MailError::ErrorInfo& error);

	int GetRetryInterval() { return m_retryInterval; }

	void SetRetryInterval(int seconds) { m_retryInterval = seconds; }

	SyncSessionPtr GetSyncSession(const MojObject& folderId);

	// Checks if the session is connected and capable of push
	bool IsPushReady(const MojObject& folder);

	// Debug method for testing
	void MagicWand(MojServiceMessage* msg, const MojObject& payload);

	// Returns true if the client is currently doing something
	bool IsActive();

	// Session calls this to indicate that is is active/inactive
	void UpdateSessionActive(ImapSession* session, bool isActive);

	static MojLogger s_log;

	// Get a human readable version of the account id
	const std::string& GetAccountIdString() const { return m_accountIdString; }

	NetworkStatusMonitor& GetNetworkStatusMonitor() const;

protected:
	typedef enum {
		State_None,
		State_NeedsAccountInfo,
		State_GettingAccount,
		State_LoginFailed,
		State_RunCommand,
		State_TerminatingSessions, 	// In progress of disabling/delete account
		State_DisableAccount,			// In progress of disabling/delete account
		State_DisablingAccount, // In progress of disabling/delete account
		State_DisabledAccount, 			// In progress of disabling/delete account
		State_DeleteAccount, 			// In progress of disabling/delete account
		State_DeletingAccount, 			// In progress of disabling/delete account
		State_DeletedAccount 			// In progress of disabling/delete account
	} ClientState;

	void SetState(ClientState state);

	ImapBusDispatcher*						m_busDispatcher;
	boost::shared_ptr<DatabaseInterface>	m_dbInterface;
	boost::shared_ptr<FileCacheClient>		m_fileCacheClient;
	MojRefCountedPtr<PowerManager>			m_powerManager;

	MojObject						m_accountId;
	boost::shared_ptr<ImapAccount> 	m_account;
	std::string						m_accountIdString;

	// change to an array
	//std::vector< ImapSessionPtr >		m_sessions; // can have multiple sessions for multiple folder syncs in parallel

	// automatically create session in constructor, stuff in array
	// always use first session in array for now. Pseudo singleton

	// sync command scenario
	// -- sync called on bus dispatcher
	// -- getOrCreateClient called with account idState_DeletingAccount
	// -- account id not found in map, new client created
	// -- get account info from database
	// -- create session
	// -- client calls sync method through session

	MojLogger&								m_log;
	MojRefCountedPtr<CommandManager>		m_commandManager;
	MojRefCountedPtr<MojServiceMessage>		m_accountEnabledMsg;
	MojObject								m_payload;
	ActivityPtr								m_activity;
	MojRefCountedPtr<MojServiceMessage>		m_deleteAccountMsg;
	std::map< const MojObject, MojRefCountedPtr<ImapSession> > m_folderSessionMap;
	std::vector< MojRefCountedPtr<ImapSession> > m_pendingDeletefolderSessions;

	NetworkStatus							m_networkStatus;

	// SyncAccount on session to check the queue and go through the login stuff

	void CheckAccountRetrieved();
	ImapSessionPtr CreateSession();
	ImapSessionPtr CreateSession(const MojObject& folderId); // used by folderSessionMap sessions

	ImapSessionPtr GetSession(const MojObject& folderId);
	ImapSessionPtr GetOrCreateSession(const MojObject& folderId);

	virtual SyncSessionPtr GetOrCreateSyncSession(const MojObject& folderId);

	void HookUpFolderSessionAccounts();
	virtual void CheckQueue();
	void RunCommandsInQueue();

	void CancelPendingCommands(ImapCommand::CancelType cancelType);

	void CleanupOneSession();
	void SendSmtpAccountEnableRequest(bool enabling);
	MojErr SmtpAccountEnabledResponse(MojObject& response, MojErr err);
	void HandleSessionCleanup();
	static gboolean CleanupSessionsCallback(gpointer data);

	static const char* GetStateName(ClientState state);

	ClientState				m_state;
	int						m_retryInterval;
	guint					m_cleanupSessionCallbackId;
	int						m_getAccountRetries;

	typedef std::map<MojObject, SyncSessionPtr> SyncSessionPtrMap;

	SyncSessionPtrMap		m_syncSessions;

	MojServiceRequest::ReplySignal::Slot<ImapClient> m_smtpAccountEnabledSlot;
};

#endif /* IMAPCLIENT_H_ */
