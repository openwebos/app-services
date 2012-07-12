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

#ifndef POPCLIENT_H_
#define POPCLIENT_H_

#include "boost/shared_ptr.hpp"
#include <glib.h>
#include <memory>
#include <vector>
#include "activity/Activity.h"
#include "activity/ActivityBuilderFactory.h"
#include "client/BusClient.h"
#include "client/Command.h"
#include "client/DownloadListener.h"
#include "client/PowerManager.h"
#include "client/PopSession.h"
#include "client/SyncSession.h"
#include "core/MojCoreDefs.h"
#include "core/MojLog.h"
#include "core/MojRefCount.h"
#include "core/MojServiceMessage.h"
#include "data/DatabaseInterface.h"
#include "data/PopAccount.h"
#include "exceptions/MojErrException.h"
#include "luna/MojLunaService.h"
#include "data/MojoDatabase.h"
#include "client/BusClient.h"
#include "client/SyncStateUpdater.h"

class PopSession;
class SyncSession;
class PopBusDispatcher;
class PopAccount;

/**
 * PopClient is a sync state machine that manages the lifecycle of all pop
 * command objects and determine when they will be executed.
 */
class PopClient : public MojRefCounted, public BusClient, public Command::Listener
{

public:
	//typedef std::vector<boost::shared_ptr<PopClientCommand::Status> > StatusList;

	typedef MojRefCountedPtr<PopSession> 				PopSessionPtr;
	typedef boost::shared_ptr<DatabaseInterface>		DBInterfacePtr;
	typedef boost::shared_ptr<PopAccount> 				AccountPtr;
	typedef MojRefCountedPtr<CommandManager> 			CommandManagerPtr;
	typedef MojRefCountedPtr<PowerManager> 				PowerManagerPtr;
	typedef MojRefCountedPtr<SyncSession> 				SyncSessionPtr;
	typedef boost::shared_ptr<FileCacheClient>			FileCacheClientPtr;

	typedef enum {
		State_None,
		State_NeedAccount,
		State_LoadingAccount,
		State_OkToRunCommands,
		State_AccountDisabled,
		State_AccountDeleted
	} State;

	static MojLogger s_log;

	//PopClient();
	PopClient(DBInterfacePtr dbInterface, PopBusDispatcher* dispatcher, const MojObject& accountId,
			  MojLunaService* service);

	PopSessionPtr CreateSession();

	/**
	 *
	 * Returns the status of each command in the client.
	 */
	//void GetStatus(Status& status);

	/**
	 * @return the database interface used to query/store objects
	 */
	DatabaseInterface& GetDatabaseInterface();

	/**
	 * @return ref counted pointer to a MojServiceRequest used for sending requests over the bus
	 */
//	MojRefCountedPtr<MojServiceRequest> CreateRequest();

	/**
	 * @return the account used to sync.
	 */
	AccountPtr 	GetAccount();

	/**
	 * @return the account ID of the account that this client manages.
	 */
	MojObject 	GetAccountId() { return m_accountId; }

	void SetBusDispatcher(PopBusDispatcher* busDispatcher) { m_popBusDispatcher = busDispatcher; }

	/**
	 * Sets the account based on the ID.
	 * Results in the client finding the account in the DB.
	 * @param the database ID of the account
	 */
	void SetAccount(MojObject& accountId);

	void UpdateMissingCredentialsSyncStatus(MojSignal<>::SlotRef slot);

	/**
	 * Sets the account to be used by the client.
	 * @param the account
	 */
	void SetAccount(AccountPtr account);

	/**
	 * Change the state to indicate that the account is failed to load.
	 */
	void FindAccountFailed(const MojObject& accountId);

	/**
	 * Enables the POP account that this client manages.
	 */
	void EnableAccount(MojServiceMessage* msg, MojObject& payload);

	/**
	 * Updates the account if the account preferences or credential changes.
	 */
	void UpdateAccount(MojObject& payload, bool credentialsChanged);

	/**
	 * Disables the POP account that this client manages.
	 */
	void DisableAccount(MojServiceMessage* msg, MojObject& payload);

	/**
	 * Deletes the POP account that this client manages.
	 */
	void DeleteAccount(MojServiceMessage* msg, MojObject& payload);

	/**
	 * A callback function for delete account command to invoke when all the
	 * resources from this account have been removed.
	 */
	void AccountDeleted();

	/**
	 * @return true if this account has been successfully deleted; false otherwise.
	 */
	bool IsAccountDeleted();

	/**
	 * Called by GLib's event loop to delete any completed commands.
	 */
	void Cleanup();

	/**
	 * Sync all folders/collections in the account with autoSync enabled.
	 */
	void SyncAccount(MojObject& payload);

	/**
	 * Syncs a given e-mail folder
	 * @param id the ID of the com.palm.folder record
	 */
	void SyncFolder(MojObject& payload);

	/**
	 * Moves emails in outbox that have been sent to SENT folder.
	 */
	void MoveOutboxFolderEmailsToSentFolder(MojServiceMessage* msg, MojObject& payload);

	/**
	 * Moves emails from source folder to destined folder.
	 */
	void MoveEmail(MojServiceMessage* msg, MojObject& payload);

	/**
	 * Download the specified attachment.
	 * @param the message used to reply with progress
	 * @param id of the email
	 * @param id of the part (which is the attachment)
	 */
	void DownloadPart(const MojObject& folderId, const MojObject& emailId, const MojObject& partId, boost::shared_ptr<DownloadListener>& listener);

	/**
	 * In case of retry-able error, we need to schedule a retry sync.
	 */
	void ScheduleRetrySync(const EmailAccount::AccountError& err);

	/**
	 * Check the state machine in this client.  If there are pending command to
	 * run, the pending commands will be run in the order of their priority and
	 * their insertion order.
	 */
	void CheckQueue();

	/**
	 * Updates the state machine if a command is completed.
	 */
	virtual void CommandComplete(Command* command);

	/**
	 * Updates the state machine if a command fails to complete.
	 */
	void		 CommandFailed(Command* command, MailError::ErrorCode errCode, const std::exception& exc);

	/**
	 * Resets the state of the client.
	 */
	void 		Reset();

	/**
	 * @return true if this client and its session still have command to run;
	 * otherwise, false.
	 */
	bool		 IsActive();

	/**
	 * A callback function to this client that POP validator and POP session tell
	 * the client that they have shut down.
	 */
	void		 SessionShutdown();

	/**
	 * Status
	 */
	void		Status(MojObject& status) const;

	PopSessionPtr		GetSession()				 { return m_session; }
	PowerManagerPtr		GetPowerManager()			 { return m_powerManager; }
	SyncSessionPtr		GetSyncSession()			 { return m_syncSession; }
	BuilderFactoryPtr 	GetActivityBuilderFactory()  { return m_builderFactory; }
	FileCacheClient& 	GetFileCacheClient() 		 { return *m_fileCacheClient.get(); }

	SyncSessionPtr		GetOrCreateSyncSession(const MojObject& folderId);
protected :
	virtual ~PopClient();

	void QueueCommand(CommandManager::CommandPtr command, bool runImmediately = false);
	void RunCommand(CommandManager::CommandPtr command);

	static const char* GetStateName(State state);

	bool					m_deleted;
	DBInterfacePtr			m_dbInterface;
	PopSessionPtr			m_session;
	State 					m_state;
	AccountPtr 				m_account;
	int						m_startedCommandTotal;
	int						m_successfulCommandTotal;
	int						m_failedCommandTotal;
	std::string				m_capabilityProvider;
	std::string				m_busAddress;
	CommandManagerPtr		m_commandManager;
	PowerManagerPtr			m_powerManager;
	SyncSessionPtr			m_syncSession;
	BuilderFactoryPtr 		m_builderFactory;

private:
	FileCacheClientPtr		m_fileCacheClient;
	MojObject				m_accountId;
	std::string				m_accountIdString;
	PopBusDispatcher*		m_popBusDispatcher;
	MojRefCountedPtr<SyncStateUpdater>	m_syncStateUpdater;
};

#endif /* POPCLIENT_H_ */
