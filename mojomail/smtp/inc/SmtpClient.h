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

#ifndef SMTPCLIENT_H_
#define SMTPCLIENT_H_

#include "boost/shared_ptr.hpp"
#include <glib.h>
#include <memory>
#include <vector>
#include <set>
#include "activity/Activity.h"
#include "client/BusClient.h"
#include "client/SmtpSession.h"
#include "commands/SmtpCommand.h"
#include "core/MojCoreDefs.h"
#include "core/MojLog.h"
#include "core/MojRefCount.h"
#include "data/DatabaseInterface.h"
#include "data/SmtpAccount.h"
#include "exceptions/MojErrException.h"
#include "luna/MojLunaService.h"
#include "client/CommandManager.h"
#include "SmtpPowerManager.h"

class FileCacheClient;
class SmtpBusDispatcher;

/**
 * SmtpClient is a sync state machine that manages the lifecycle of all smtp
 * command objects and determine when they will be executed.
 */
class SmtpClient : public MojRefCounted, public BusClient, public Command::Listener
{
public:
	typedef std::vector<boost::shared_ptr<SmtpCommand::CommandStats> > StatusList;

	typedef enum {
		State_None,
		State_UsernameRequired,
		State_PasswordRequired,
		State_PendingLogin,
		State_OkToSync,
		State_InPing,
		State_InvalidCredentials,
		State_AccountDeleted
	} State;

	class ClientStatus
	{
	public:
		ClientStatus();

		StatusList m_pending;
		StatusList m_active;
		StatusList m_completed;
		int	m_startedTotal;
		int m_successfulTotal;
		int m_failedTotal;
		State m_state;
	};

	static MojLogger s_log;

	//SmtpClient();
	SmtpClient(SmtpBusDispatcher* smtpBusDispatcher, boost::shared_ptr<DatabaseInterface> dbInterface, boost::shared_ptr<DatabaseInterface> tempDbInterface,
			  MojLunaService* service);
	virtual ~SmtpClient();

	boost::shared_ptr<SmtpSession> GetSession();

	/**
	 *
	 * Returns the status of each command in the client.
	 */
	void GetStatus(ClientStatus& status);

	/**
	 * @return the database interface used to query/store objects
	 */
	DatabaseInterface& GetDatabaseInterface(){return *m_dbInterface.get();}

	DatabaseInterface& GetTempDatabaseInterface() { return *m_tempDbInterface.get(); }

	FileCacheClient&   GetFileCacheClient(){return *m_fileCacheClient.get(); }

	/**
	 * @return ref counted pointer to a MojServiceRequest used for sending requests over the bus
	 */
	MojRefCountedPtr<MojServiceRequest> CreateRequest();

	/**
	 * Sets the account based on the ID.
	 * Results in the client finding the account in the DB.
	 * @param the database ID of the account
	 * @param if true the account will sync once it has been retrieved from the DB
	 */
	void SetAccountId(MojObject& accountId);

	/*
	 * Returns the account id for this client.
	 */
	const MojObject& GetAccountId() const { return m_accountId; }

	void UpdateAccount(MojRefCountedPtr<Activity>, const MojObject& activityId, const MojObject& activityName, const MojObject& accountId);

	/**
	 * Enables an account.
	 */
	void EnableAccount(const MojRefCountedPtr<MojServiceMessage>& msg);

	/**
	 * Cancels all active commands and deletes all data associated with the account.
	 */
	void DisableAccount(const MojRefCountedPtr<MojServiceMessage>& msg);


	/**
	 * @return the power manager used for keeping the device awake
	 */
	PowerManager& GetPowerManager();
                                  
	void SaveEmail(const MojRefCountedPtr<MojServiceMessage> msg, MojObject& email, MojObject& accountId, bool isDraft);
	
	void SyncOutbox(MojRefCountedPtr<Activity> activity, const MojString& activityId, const MojString& activityName, MojObject& accountId, MojObject& folderId, bool force);

	void RemoveWatches(const MojRefCountedPtr<MojServiceMessage> msg, MojObject& accountId);

	void CommandComplete(Command* command);

	/**
	 * Handles failure of a command
	 */
	void CommandFailure(SmtpCommand* command);
	/**
	 * Feeder function for command failure
	 */
	void CommandFailure(SmtpCommand* command, const std::exception& e);

	/**
	 * Called by GLib's event loop to delete any completed commands.
	 */
	void Cleanup();

	bool IsActive();
	void SessionShutdown();

	/**
	 * @throw MojoDbException if err != MojErrNone
	 */
	static void CheckError(MojErr err, const char* file, int line) {
		if (err)
			throw MojErrException(err, file, line);
	}

	void Status(MojObject& status) const;

protected :
	typedef MojRefCountedPtr<SmtpCommand> CommandPtr;
	typedef std::vector<CommandPtr> 	 CommandList;
	
	// FIXME: This raw set isn't clean.	
	std::set<Command*>					m_syncCommands;

	SmtpBusDispatcher*						m_smtpBusDispatcher;

	// a database interface to read, merge or delete mojo objects.
	boost::shared_ptr<DatabaseInterface>	m_dbInterface;
	boost::shared_ptr<DatabaseInterface>	m_tempDbInterface;
	boost::shared_ptr<FileCacheClient>		m_fileCacheClient;

	// a state machine to run smtp commands
	boost::shared_ptr<SmtpSession>			m_session;
	MojRefCountedPtr<CommandManager>		m_commandManager;
	State 									m_state;
	int										m_startedCommandTotal;
	int										m_successfulCommandTotal;
	int										m_failedCommandTotal;
	CommandList								m_pendingCommands;
	CommandList				 				m_activeCommands;
	CommandList								m_completeCommands;
	MojRefCountedPtr<SmtpPowerManager>		m_powerManager;
private:
	MojObject								m_accountId;
};

#endif /* SMTPCLIENT_H_ */
