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

#include "SmtpClient.h"
#include "SmtpCommon.h"
#include "commands/SaveEmailCommand.h"
#include "commands/SmtpSendMailCommand.h"
#include "commands/SmtpAccountDisableCommand.h"
#include "commands/SmtpAccountEnableCommand.h"
#include "commands/SmtpSyncOutboxCommand.h"
#include "client/FileCacheClient.h"
#include "SmtpBusDispatcher.h"

using namespace boost;
using namespace std;

MojLogger SmtpClient::s_log("com.palm.smtp");

SmtpClient::ClientStatus::ClientStatus()
: m_startedTotal(0),
  m_successfulTotal(0),
  m_failedTotal(0),
  m_state(State_None)
{

}

SmtpClient::SmtpClient(SmtpBusDispatcher* smtpBusDispatcher, shared_ptr<DatabaseInterface> dbInterface, shared_ptr<DatabaseInterface> tempDbInterface, MojLunaService* service)
: BusClient(service),
  m_smtpBusDispatcher(smtpBusDispatcher),
  m_dbInterface(dbInterface),
  m_tempDbInterface(tempDbInterface),
  m_fileCacheClient(new FileCacheClient(*this)),
  m_session(new SmtpSession(service)),
  m_commandManager(new CommandManager()),
  m_state(State_None),
  m_startedCommandTotal(0),
  m_successfulCommandTotal(0),
  m_failedCommandTotal(0),
  m_powerManager(new SmtpPowerManager(*this))
{
	m_session->SetDatabaseInterface(m_dbInterface);
	m_session->SetFileCacheClient(m_fileCacheClient);
	m_session->SetClient(this);
}

SmtpClient::~SmtpClient()
{
}

boost::shared_ptr<SmtpSession> SmtpClient::GetSession() {
	return m_session;
}

void SmtpClient::GetStatus(ClientStatus& status)
{
	status.m_state = m_state;
	status.m_startedTotal = m_startedCommandTotal;
	status.m_successfulTotal = m_successfulCommandTotal;
	status.m_failedTotal = m_failedCommandTotal;
}


MojRefCountedPtr<MojServiceRequest> SmtpClient::CreateRequest()
{
	MojLogTrace(s_log);

	assert(m_service);
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);
	ErrorToException(err);
	return req;
}

void SmtpClient::SetAccountId(MojObject& accountId)
{
	MojLogTrace(s_log);

	m_accountId = accountId;
	m_session->SetAccountId(accountId);
}

// TODO: Need to use ActivitySet for activity handling.
void SmtpClient::UpdateAccount(MojRefCountedPtr<Activity> activity, const MojObject& activityId, const MojObject& activityName, const MojObject& accountId)
{
	// We force the outbox sync, to bypass a lockout-error, such as bad credentials. The sync will clear any
	// error when it starts, and also reset the account.
	MojLogInfo(s_log, "Constructing new SyncOutbox command to adopt activity after account re-fetch");
	MojRefCountedPtr<SmtpSyncOutboxCommand> syncCommand(new SmtpSyncOutboxCommand(*this, accountId, MojObject(), true /*force*/, true /*clear*/));
	m_syncCommands.insert(syncCommand.get());
	m_commandManager->QueueCommand(syncCommand, true);

	// Attach our activity to that sync
	MojString activityId_s, activityName_s;
	activityId.stringValue(activityId_s);
	activityName.stringValue(activityName_s);
	syncCommand->AttachAdoptedActivity(activity, activityId_s, activityName_s);

}

void SmtpClient::EnableAccount(const MojRefCountedPtr<MojServiceMessage>& msg)
{
	MojLogNotice(s_log, "enabling SMTP for account %s", AsJsonString(m_accountId).c_str());

	MojRefCountedPtr<SmtpAccountEnableCommand> enableCommand(new SmtpAccountEnableCommand(*this, msg));
	m_commandManager->QueueCommand(enableCommand, true);
}

void SmtpClient::DisableAccount(const MojRefCountedPtr<MojServiceMessage>& msg)
{
	// Enqueue command to wipe out watches, both account watches and outbox watches, if any..
	MojLogInfo(s_log, "disabling SMTP for account %s", AsJsonString(m_accountId).c_str());
	MojRefCountedPtr<SmtpAccountDisableCommand> disableCommand(new SmtpAccountDisableCommand(*this, msg, m_accountId));
	m_commandManager->QueueCommand(disableCommand, true);
}

void SmtpClient::SaveEmail(const MojRefCountedPtr<MojServiceMessage> msg, MojObject& email, MojObject& accountId, bool isDraft)
{
	MojRefCountedPtr<SaveEmailCommand> command(new SaveEmailCommand(*this, msg, email, accountId, isDraft));
	m_commandManager->QueueCommand(command, true);
}

// TODO: Need to use ActivitySet for activity handling.
void SmtpClient::SyncOutbox(MojRefCountedPtr<Activity> activity, const MojString& activityId, const MojString& activityName, MojObject& accountId, MojObject& folderId, bool force)
{
	bool adopted = false;
	
	MojLogInfo(s_log, "Syncing outbox");
	
	// FIXME: Should remove set of raw pointers and instead allow CommandManager to expose
	// active commands, or allow matching against predicates.

	for (std::set<Command*>::iterator i = m_syncCommands.begin(); i != m_syncCommands.end(); i++) {
		SmtpSyncOutboxCommand * cmd = (SmtpSyncOutboxCommand*)*i;
		if (cmd->GetAccountId() == accountId && cmd->CanAdopt()) {
			MojLogInfo(s_log, "Telling existing SyncOutbox command to adopt activity");
			cmd->AttachAdoptedActivity(activity, activityId, activityName);
			adopted = true;
			break;
		}
	}
	
	if (!adopted) {
		MojLogInfo(s_log, "Constructing new SyncOutbox command to adopt activity");
		
		MojRefCountedPtr<SmtpSyncOutboxCommand> command(new SmtpSyncOutboxCommand(*this, accountId, folderId, force, false /*clear*/));
		m_syncCommands.insert(command.get());
		
		m_commandManager->QueueCommand(command, true);
		command->AttachAdoptedActivity(activity, activityId, activityName);
	}
}

void SmtpClient::Cleanup()
{
	MojLogTrace(s_log);
}

void SmtpClient::CommandComplete(Command* command)
{
	MojLogInfo(s_log, "Completing command, removing aux");
			
	m_syncCommands.erase(command);
	
	m_commandManager->CommandComplete(command);
	m_commandManager->RunCommands();

	// shutdown if pending & active command lists are empty
	SessionShutdown();
}

/**
 * Handles failure of a command
 */
void SmtpClient::CommandFailure(SmtpCommand* command)
{
	assert(command);
	CommandComplete(command);
}
/**
 * Feeder function for command failure
 */
void SmtpClient::CommandFailure(SmtpCommand* command, const std::exception& e)
{
	MojLogTrace(s_log);
	MojLogError(s_log, "command failure, exception: %s", e.what());
	CommandFailure(command);
}

PowerManager& SmtpClient::GetPowerManager()
{
        return *(m_powerManager.get());
}

bool SmtpClient::IsActive()
{
	return m_commandManager->GetActiveCommandCount() > 0
		   || m_commandManager->GetPendingCommandCount() > 0
		   || !m_session->IsReadyForShutdown();
}

void SmtpClient::SessionShutdown()
{
	if (m_commandManager->GetActiveCommandCount() == 0 &&
		m_commandManager->GetPendingCommandCount() == 0) {
		m_smtpBusDispatcher->PrepareShutdown();
	}
}

void SmtpClient::Status(MojObject& status) const
{
	MojErr err;

	err = status.put("accountId", m_accountId);
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
