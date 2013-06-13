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

#include "client/ImapSession.h"
#include <boost/lexical_cast.hpp>
#include "data/ImapAccount.h"
#include "data/ImapLoginSettings.h"

#include "exceptions/MailException.h"
#include "client/ImapRequestManager.h"
#include <sstream>
#include "ImapCoreDefs.h"
#include "ImapPrivate.h"
#include "client/SyncSession.h"
#include "client/PowerManager.h"
#include "exceptions/ExceptionUtils.h"
#include "activity/ImapActivityFactory.h"
#include "activity/ActivityBuilder.h"
#include "network/NetworkStatus.h"
#include "network/NetworkStatusMonitor.h"
#include "stream/InflaterInputStream.h"
#include "stream/DeflaterOutputStream.h"

// Commands
#include "commands/AutoDownloadCommand.h"
#include "commands/CapabilityCommand.h"
#include "commands/CompressCommand.h"
#include "commands/ConnectCommand.h"
#include "commands/FetchNewHeadersCommand.h"
#include "commands/FetchPartCommand.h"
#include "commands/IdleCommand.h"
#include "commands/IdleYahooCommand.h"
#include "commands/LoginCommand.h"
#include "commands/LogoutCommand.h"
#include "commands/AuthYahooCommand.h"
#include "commands/NoopIdleCommand.h"
#include "commands/SearchFolderCommand.h"
#include "commands/SelectFolderCommand.h"
#include "commands/ScheduleRetryCommand.h"
#include "commands/SyncFolderListCommand.h"
#include "commands/SyncEmailsCommand.h"
#include "commands/SyncLocalChangesCommand.h"
#include "commands/TLSCommand.h"
#include "commands/UpSyncSentEmailsCommand.h"
#include "commands/UpSyncDraftsCommand.h"

using namespace std;

#include <string>
#include "ImapCoreDefs.h"
#include "ImapConfig.h"

// Read timeout for normal requests
const int ImapSession::DEFAULT_REQUEST_TIMEOUT = 30; // 30 seconds

// If we can't use IDLE, schedule a sync with this interval
const int ImapSession::FALLBACK_SYNC_INTERVAL = 15 * 60; // 15 minutes

// How long we can keep the power on in a transitory state
const int ImapSession::STATE_POWER_TIMEOUT = 5 * 60; // 5 minutes

// How long we can keep the power on in the OkToSync state
const int ImapSession::SYNC_STATE_POWER_TIMEOUT = 30 * 60; // 30 minutes

MojLogger ImapSession::s_log("com.palm.imap.session");

std::vector< MojRefCountedPtr<MojRefCounted> > ImapSession::s_asyncCleanupItems;
guint ImapSession::s_asyncCleanupCallbackId = 0;

ImapSession::ImapSession(const MojRefCountedPtr<ImapClient>& client, MojLogger& logger)
: m_log(logger),
  m_dbInterface(NULL),
  m_fileCacheClient(NULL),
  m_busClient(NULL),
  m_commandManager(new CommandManager(1, true)), // only one command at a time; start paused
  m_requestManager(new ImapRequestManager(*this)),
  m_state(State_NeedsAccount),
  m_client(client),
  m_activitySet(NULL),
  m_recentUserInteraction(false),
  m_reconnectRequested(false),
  m_safeMode(false),
  m_compressionActive(false),
  m_shouldPush(false),
  m_idleMode(IdleMode_None),
  m_pushRetryCount(0),
  m_enteredStateTime(0),
  m_idleStartTime(0),
  m_sessionPushDuration(0),
  m_networkStatusSlot(this, &ImapSession::NetworkStatusAvailable),
  m_closedSlot(this, &ImapSession::ConnectionClosed),
  m_syncSessionDoneSlot(this, &ImapSession::SyncSessionDone),
  m_scheduleRetrySlot(this, &ImapSession::ScheduleRetryDone),
  m_activitiesEndedSlot(this, &ImapSession::ActivitiesEnded)
{
}

ImapSession::~ImapSession()
{
}

void ImapSession::SetDatabase(DatabaseInterface& dbInterface)
{
	m_dbInterface = &dbInterface;
}

void ImapSession::SetFileCacheClient(FileCacheClient& fileCacheClient)
{
	m_fileCacheClient = &fileCacheClient;
}

void ImapSession::SetBusClient(BusClient& busClient)
{
	m_busClient = &busClient;
}

void ImapSession::SetPowerManager(const MojRefCountedPtr<PowerManager>& powerManager)
{
	m_powerManager = powerManager;
}

void ImapSession::Connected(const MojRefCountedPtr<SocketConnection>& connection)
{
	m_connection = connection;
	
	if(connection.get()) {
		m_inputStream = connection->GetInputStream();
		m_outputStream = connection->GetOutputStream();

		m_closedSlot.cancel(); // make sure it's not connected to a slot
		m_connection->WatchClosed(m_closedSlot);

		if(NeedLoginCapabilities()) {
			CheckLoginCapabilities();
		} else {
			LoginCapabilityComplete();
		}
	} else {
		CheckQueue();
	}
}

bool ImapSession::NeedLoginCapabilities()
{
	/* As an optimization, we don't bother checking the login capabilities unless needed.
	 *
	 * Note that we can run into problems by not checking for LOGINDISABLED; we may
	 * incorrectly report "bad username/password" instead of "TLS required".
	 *
	 * ImapValidator should override this to always return true to avoid this problem.
	 */

	ImapLoginSettings::EncryptionType encryption = GetLoginSettings().GetEncryption();

	if(encryption == ImapLoginSettings::Encrypt_TLSIfAvailable) {
		return true;
	}

	return false;
}

void ImapSession::CheckLoginCapabilities()
{
	SetState(State_GettingLoginCapabilities);

	MojRefCountedPtr<CapabilityCommand> command(new CapabilityCommand(*this));
	m_commandManager->RunCommand(command);
}

void ImapSession::LoginCapabilityComplete()
{
	// Enable TLS if requested
	ImapLoginSettings::EncryptionType encryption = GetLoginSettings().GetEncryption();
	bool supportsTLS = m_capabilities.HasCapability(Capabilities::STARTTLS);

	if(encryption == ImapLoginSettings::Encrypt_TLS ||
		(encryption == ImapLoginSettings::Encrypt_TLSIfAvailable && supportsTLS)) {
		RunState(State_NeedsTLS);
	} else {
		RunState(State_LoginRequired);
	}
}

void ImapSession::TLSReady()
{
	RunState(State_LoginRequired);
}

void ImapSession::SetAccount(boost::shared_ptr<ImapAccount>& account) {
	m_account = account;
	if(m_state == State_NeedsAccount) {
		MojLogDebug(m_log, "ImapSession %p set account object", this);
		SetState(State_NeedsConnection);

		if(m_commandManager->GetPendingCommandCount() > 0)
			CheckQueue();
	} else {
		MojLogDebug(m_log, "ImapSession %p updated account object", this);
	}
}

void ImapSession::RunState(State newState)
{
	SetState(newState);
	CheckQueue();
}

void ImapSession::SetFolder(const MojObject& folderId)
{
	m_folderId = folderId;
}

bool ImapSession::NeedsPowerInState(State state) const
{
	switch(state) {
	case State_NeedsAccount:
	case State_NeedsConnection:
	case State_Idling:
	case State_InvalidCredentials:
	case State_AccountDeleted:
		return false;
	default:
		return true;
	}
}

bool ImapSession::IsConnected() const
{
	switch(m_state) {
	case State_NeedsAccount:
	case State_NeedsConnection:
	case State_QueryingNetworkStatus:
	case State_Connecting:
	case State_InvalidCredentials:
	case State_AccountDeleted:
	case State_Disconnecting:
	case State_Cleanup:
		return false;
	default:
		return true;
	}
}

bool ImapSession::IsActive() const
{
	if(m_commandManager->GetActiveCommandCount() > 0)
		return true;

	switch(m_state) {
	case State_NeedsConnection:
	case State_AccountDeleted:
	case State_InvalidCredentials:
		return false;
	default:
		break;
	}

	// Always active when connected to the server
	return true;
}

void ImapSession::SetState(State newState)
{
	MojLogDebug(m_log, "changing session %p state from %s (%d) to %s (%d)", this, GetStateName(m_state), m_state, GetStateName(newState), newState);
	m_state = newState;

	if(newState != State_OkToSync) {
		// Disable running commands for any state other than State_OkToSync
		m_commandManager->Pause();
	}

	m_stateTimer.Cancel();

	if(NeedsPowerInState(m_state)) {
		// These states need power
		if(HasPowerManager()) {
			m_powerUser.Start(GetPowerManager(), "ImapSession active");
		}

		// Kill power activity if it's stuck for more than 30 minutes
		m_stateTimer.SetTimeout(m_state == State_OkToSync ? SYNC_STATE_POWER_TIMEOUT : STATE_POWER_TIMEOUT,
				this, &ImapSession::StateTimeout);
	} else {
		// These states don't need power
		m_powerUser.Stop();
	}

	if(m_client.get()) {
		// Tell the client that we're active/inactive
		m_client->UpdateSessionActive(this, IsActive());
	}

	m_enteredStateTime = time(NULL);
}

void ImapSession::StateTimeout()
{
	MojLogCritical(m_log, "ImapSession %p spent over %d minutes in state %s", this,
			(int)(time(NULL) - m_enteredStateTime)/60, GetStateName(m_state));
}

void ImapSession::DisconnectSession()
{
	if(IsConnected()) {
		MojLogInfo(m_log, "disconnecting session %p; current state: %s (%d)", this, GetStateName(m_state), m_state);

		SetState(State_Disconnecting);
		Disconnect();
	} else {
		MojLogInfo(m_log, "session %p not connected", this);

		m_client->SessionDisconnected(m_folderId);
	}
}

const boost::shared_ptr<ImapAccount>& ImapSession::GetAccount()
{
	return m_account;
}

InputStreamPtr& ImapSession::GetInputStream()
{
	assert( m_inputStream.get() != NULL );

	if(m_inputStream.get() != NULL) {
		return m_inputStream;
	} else {
		throw MailException("no input stream available", __FILE__, __LINE__);
	}
}

OutputStreamPtr& ImapSession::GetOutputStream()
{
	assert( m_outputStream.get() != NULL );

	if(m_outputStream.get() != NULL) {
		return m_outputStream;
	} else {
		throw MailException("no output stream available", __FILE__, __LINE__);
	}
}

const MojRefCountedPtr<SocketConnection>& ImapSession::GetConnection()
{
	return m_connection;
}

bool ImapSession::CheckNetworkHealthForPush()
{
	const NetworkStatus& networkStatus = m_client->GetNetworkStatusMonitor().GetCurrentStatus();

	// FIXME: need to know which interface we're currently on
	if(networkStatus.IsKnown() && !ImapConfig::GetConfig().GetIgnoreNetworkStatus()) {
		boost::shared_ptr<InterfaceStatus> interfaceStatus = networkStatus.GetPersistentInterface();
		if(interfaceStatus.get() != NULL) {
			InterfaceStatus::NetworkConfidence confidence = interfaceStatus->GetNetworkConfidence();
			if(confidence == InterfaceStatus::POOR) {
				MojLogInfo(m_log, "can't push; poor network confidence");
				return false;
			}
		} else {
			MojLogInfo(m_log, "can't push; no persistent interface");
			return false;
		}
	} else {
		MojLogInfo(m_log, "network status unknown");
	}

	return true;
}

int ImapSession::GetNoopIdleTimeout()
{
	int keepAliveSeconds = 0;

	if(ImapConfig::GetConfig().GetKeepAliveForSync()
	&& !m_account->IsYahoo()
	&& m_folderId == m_account->GetInboxFolderId()
	&& m_account->GetSyncFrequencyMins() > 0
	&& m_account->GetSyncFrequencyMins() <= 15) {
		// If keepAliveForSync is enabled, stay connected for up to 16 minutes waiting for the next sync.
		keepAliveSeconds = (m_account->GetSyncFrequencyMins() * 60) + 60;
	} else if(ImapConfig::GetConfig().GetSessionKeepAlive() > 0 && m_recentUserInteraction) {
		// If the user has recently interacted with the folder, keep it alive
		keepAliveSeconds = ImapConfig::GetConfig().GetSessionKeepAlive();

		if(m_account->IsYahoo()) {
			// Max of 25 seconds for Yahoo, which doesn't like us to stay connected
			keepAliveSeconds = std::min(keepAliveSeconds, 25);
		}
	}

	return keepAliveSeconds;
}

void ImapSession::CommandComplete(Command* command)
{
	m_commandManager->CommandComplete(command);

	if(m_reconnectRequested && m_state == State_OkToSync && m_commandManager->GetActiveCommandCount() == 0 && m_commandManager->GetPendingCommandCount() > 0) {
		// If requested (and no active commands), disconnect from the server after finishing this command
		MojLogInfo(m_log, "disconnecting and reconnecting to server");

		Logout();
	} else if(m_state == State_OkToSync && m_commandManager->GetPendingCommandCount() == 0
			&& m_commandManager->GetActiveCommandCount() == 0) {
		MojLogInfo(m_log, "no commands active or pending");

		// Either disconnect or run IDLE command
		// TODO also check account settings

		if(IsValidId(m_folderId) && IsPushEnabled(m_folderId)) {
			m_shouldPush = CheckNetworkHealthForPush();
		} else {
			m_shouldPush = false;
		}

		if(m_shouldPush) {
			//MojLogInfo(m_log, "running idle command for folderId %s", AsJsonString(m_folderId).c_str());
			if(m_account->IsYahoo()) {
				m_idleCommand.reset(new IdleYahooCommand(*this, m_folderId));
				m_idleMode = IdleMode_YahooPush;
			} else {
				m_idleCommand.reset(new IdleCommand(*this, m_folderId));
				m_idleMode = IdleMode_IDLE;
			}

			// Create activity to maintain idle.
			// If the device is rebooted or IMAP crashes, the callback will restart idle.
			// The activity will get cancelled if the connection goes into retry mode.
			if(true) {
				ImapActivityFactory factory;
				ActivityBuilder ab;

				factory.BuildStartIdle(ab, m_client->GetAccountId(), m_folderId);

				// Create new activity only if one doesn't already exist
				if(GetActivitySet()->FindActivityByName(ab.GetName()).get() == NULL) {
					ActivityPtr activity = Activity::PrepareNewActivity(ab, true, true);
					activity->SetName(ab.GetName());

					activity->SetEndAction(Activity::EndAction_Complete);
					activity->SetEndOrder(Activity::EndOrder_Last);
					GetActivitySet()->AddActivity(activity);

					// Start activity right away
					activity->Create(*m_client);
				}
			}

			m_idleStartTime = 0;

			SetState(State_PreparingToIdle);
			m_commandManager->RunCommand(m_idleCommand);
		} else if(GetNoopIdleTimeout() > 0) {
			// TODO: this code is currently disabled
			MojLogInfo(m_log, "running NOOP idle command");
			m_idleCommand.reset(new NoopIdleCommand(*this, m_folderId, GetNoopIdleTimeout()));

			m_idleMode = IdleMode_NOOP;
			m_idleStartTime = 0;

			// Reset flag
			m_recentUserInteraction = false;

			SetState(State_PreparingToIdle);
			m_commandManager->RunCommand(m_idleCommand);

		} else {
			if(IsPushRequested(m_folderId) && !m_shouldPush) {
				MojLogInfo(m_log, "setting up scheduled sync instead of push");

				// If we can't push, set up a scheduled sync
				ImapActivityFactory factory;
				ActivityBuilder ab;

				factory.BuildScheduledSync(ab, m_client->GetAccountId(), m_folderId, FALLBACK_SYNC_INTERVAL, false);

				GetActivitySet()->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
			} else {
				MojLogInfo(m_log, "nothing left to do; disconnecting");
			}

			Logout();
		}
	} else if(m_state == State_Disconnecting && m_commandManager->GetActiveCommandCount() == 0) {
		Disconnected();
	}
}

// Report command failure. After reporting a failure, the caller should call CommandComplete().
void ImapSession::CommandFailure(Command* command, const exception& e)
{
}

void ImapSession::CheckQueue()
{
	MojLogTrace(m_log);

	MojLogDebug(m_log, "checking the session queue. current state: %s (%d). commands active: %d pending: %d",
			GetStateName(m_state), m_state, m_commandManager->GetActiveCommandCount(), m_commandManager->GetPendingCommandCount());

	MojAssert( m_state == State_NeedsAccount || m_account.get() );

	switch(m_state)
	{
		case State_NeedsAccount:
		{
			MojLogCritical(m_log, "unable to run commands, no account available");
			break;
		}
		case State_NeedsConnection:
		{
			PrepareToConnect();

			break;
		}
		case State_QueryingNetworkStatus:
		{
			break;
		}
		case State_Connecting:
		{
			break;
		}
		case State_NeedsTLS:
		{
			SetState(State_TLSNegotiation);

			MojLogInfo(m_log, "starting TLS negotiation");
			MojRefCountedPtr<TLSCommand> command(new TLSCommand(*this));
			m_commandManager->RunCommand(command);
			break;
		}
		case State_TLSNegotiation:
		{
			break;
		}
		case State_LoginRequired:
		{
			SetState(State_PendingLogin);
			
			if(IsYahoo())
			{
				//MojLogInfo(m_log, "running Yahoo authentication command");
				MojRefCountedPtr<AuthYahooCommand> command(new AuthYahooCommand(*this));
				m_commandManager->RunCommand(command);
			}
			else
			{
				//MojLogInfo(m_log, "running login command");
				MojRefCountedPtr<LoginCommand> command(new LoginCommand(*this));
				m_commandManager->RunCommand(command);
			}

			break;
		}
		case State_PendingLogin:
		{
			break;
		}
		case State_GettingCapabilities:
		{
			break;
		}
		case State_RequestingCompression:
		{
			break;
		}
		case State_SelectingFolder:
		{
			break;
		}
		case State_OkToSync:
		{
			MojLogDebug(m_log, "checking session command queue");
			m_commandManager->Resume();

			break;
		}
		case State_PreparingToIdle:
		{
			// FIXME -- possibly call endIdle
			break;
		}
		case State_Idling:
		{
			if(m_commandManager->GetPendingCommandCount() > 0) {
				if(m_idleCommand.get()) {
					MojLogInfo(m_log, "ending idle; we have work to do (pending command: %s)",
							m_commandManager->Top()->Describe().c_str());
					m_idleCommand->EndIdle();
					m_idleCommand.reset();
				}
			}
			break;
		}
		case State_InvalidCredentials:
		{
			//todo
			break;
		}
		case State_Disconnecting:
		{
			int numActiveCommands = m_commandManager->GetActiveCommandCount();

			if(numActiveCommands == 0) {
				Disconnected();
			} else {
				MojLogInfo(m_log, "session %p waiting for %d %s to finish", this, numActiveCommands,
										numActiveCommands > 1 ? "commands" : "command");
			}
			break;
		}
		case State_Cleanup:
		{
			break;
		}
		case State_AccountDeleted:
		{
			//
			break;
		}
		default:
		{
			MojLogError(m_log, "unhandled session state %d", m_state);
		}
	}
}

void ImapSession::RunCommandsInQueue()
{
	assert( m_account.get() != NULL );
	m_commandManager->RunCommands();
}

void ImapSession::SendRequest(const string& request, MojRefCountedPtr<ImapResponseParser> responseParser, int timeout, bool canLogRequest)
{
	assert( m_requestManager.get() != NULL );
	
	m_requestManager->SendRequest(request, responseParser, timeout, canLogRequest);
}

void ImapSession::UpdateRequestTimeout(const MojRefCountedPtr<ImapResponseParser>& responseParser, int timeoutSeconds)
{
	m_requestManager->UpdateRequestTimeout(responseParser, timeoutSeconds);
}

LineReaderPtr& ImapSession::GetLineReader()
{
	if(m_lineReader.get() == NULL)
		m_lineReader.reset( new LineReader(GetInputStream()) );
	return m_lineReader;
}

void ImapSession::FolderListSynced()
{
	// no longer does anything
}

void ImapSession::FolderSelected()
{
	if(m_state == State_SelectingFolder) {
		RunState(State_OkToSync);
	} else {
		MojLogError(m_log, "%s called in wrong state", __PRETTY_FUNCTION__);
	}
}

void ImapSession::FolderSelectFailed()
{
	// Turn on safe mode (disables asynchronous folder list syncing)
	m_safeMode = true;

	// FIXME handle possibly non-existent folder

	if(m_state == State_SelectingFolder) {
		SetState(State_Disconnecting);
		Disconnect();
	}
}

void ImapSession::SyncAccount()
{
	// FIXME Generally this doesn't do anything if the account is already sync'd
	CheckQueue();
}

void ImapSession::SyncFolder(const MojObject& folderId, SyncParams syncParams)
{
	// TODO: make sure this is the right folder, or switch folders if necessary

	MojObject status;
	syncParams.Status(status);
	MojLogInfo(m_log, "ImapSession::SyncFolder called on %s with sync params %s", AsJsonString(folderId).c_str(), AsJsonString(status).c_str());

	// Set bind address for interface to connect to next time we connect.
	// This won't apply if we're already connected.
	if(!IsConnected() && !syncParams.GetBindAddress().empty()) {
		m_connectBindAddress = syncParams.GetBindAddress();
	}

	if(syncParams.GetForceReconnect()) {
		m_reconnectRequested = true;
	}

	// Manual sync of the inbox should resync the folder list
	if(syncParams.GetForce() && m_account->GetInboxFolderId() == folderId) {
		syncParams.SetSyncFolderList(true);
	}

	// Check if we should re-sync the folder list
	if(syncParams.GetSyncFolderList() && IsConnected()) {
		MojRefCountedPtr<SyncFolderListCommand> command(new SyncFolderListCommand(*this));
		m_commandManager->QueueCommand(command, false);
	}

	MojRefCountedPtr<ImapSyncSessionCommand> syncCommand;
	syncCommand.reset(new SyncEmailsCommand(*this, folderId, syncParams));

	// Check if a similar request has already been queued
	MojRefCountedPtr<ImapCommand> pendingCommand = FindPendingCommand(syncCommand);

	if (pendingCommand.get()) {
		MojLogInfo(m_log, "debouncing sync request; a similar request is already queued");
		CheckQueue();
		return;
	}

	// If we're just syncing local changes, and there's already a sync session, debounce it.
	if (syncParams.GetSyncChanges() && !syncParams.GetSyncEmails()) {
		MojRefCountedPtr<SyncSession> syncSession = m_client->GetSyncSession(folderId);

		if (syncSession.get() && syncSession->IsActive()) {
			MojLogInfo(m_log, "debouncing sync local changes; a sync session is active");
			return;
		}
	}

	if(syncParams.GetSyncChanges() && !syncParams.GetSyncEmails()) {
		// Just upload changes
		syncCommand.reset(new SyncLocalChangesCommand(*this, folderId));
		m_commandManager->QueueCommand(syncCommand, false);

		m_recentUserInteraction = true;
	} else if(syncParams.GetSyncEmails()) {
		// Sync emails
		m_commandManager->QueueCommand(syncCommand, false);
	} else {
		// just check the queue
		syncCommand.reset();
	}

	if(syncCommand.get()) {
		// Start sync session
		m_client->AddToSyncSession(m_folderId, syncCommand.get());
		m_client->StartSyncSession(m_folderId, syncParams.GetSyncActivities());
	}

	CheckQueue();
}

void ImapSession::UpSyncSentEmails(const MojObject& folderId, const ActivityPtr& activity)
{
	MojRefCountedPtr<UpSyncSentEmailsCommand> command(new UpSyncSentEmailsCommand(*this, folderId));
	command->AddActivity(activity);
	m_commandManager->QueueCommand(command, false);
	CheckQueue();
}

void ImapSession::UpSyncDrafts(const MojObject& folderId, const ActivityPtr& activity)
{
	MojRefCountedPtr<UpSyncDraftsCommand> command(new UpSyncDraftsCommand(*this, folderId));
	command->AddActivity(activity);
	m_commandManager->QueueCommand(command, false);
	CheckQueue();
}

MojRefCountedPtr<ImapCommand> ImapSession::FindActiveCommand(const MojRefCountedPtr<ImapCommand>& needle)
{
	ImapCommand& needleRef = *needle;
	ImapCommand::CommandType type = needleRef.GetType();

	// No command can match unknown
	if(type == ImapCommand::CommandType_Unknown) {
		return NULL;
	}

	BOOST_FOREACH(const MojRefCountedPtr<Command>& activeCommand, m_commandManager->GetActiveCommandIterators()) {
		// Assume it's an ImapCommand
		ImapCommand* imapCommand = static_cast<ImapCommand*>(activeCommand.get());

		if(imapCommand->GetType() == type) {
			if(imapCommand->Equals(needleRef)) {
				return MojRefCountedPtr<ImapCommand>(imapCommand);
			}
		}
	}

	return NULL;
}

MojRefCountedPtr<ImapCommand> ImapSession::FindPendingCommand(const MojRefCountedPtr<ImapCommand>& needle)
{
	ImapCommand& needleRef = *needle;
	ImapCommand::CommandType type = needleRef.GetType();

	// No command can match unknown
	if(type == ImapCommand::CommandType_Unknown) {
		return NULL;
	}

	BOOST_FOREACH(const MojRefCountedPtr<Command>& pendingCommand, m_commandManager->GetPendingCommandIterators()) {
		// Assume it's an ImapCommand
		ImapCommand* imapCommand = static_cast<ImapCommand*>(pendingCommand.get());

		if(imapCommand->Equals(needleRef)) {
			return MojRefCountedPtr<ImapCommand>(imapCommand);
		}
	}

	return NULL;
}

void ImapSession::DownloadPart(const MojObject& folderId, const MojObject& emailId, const MojObject& partId, const MojRefCountedPtr<DownloadListener>& listener, Command::Priority priority)
{
	MojRefCountedPtr<FetchPartCommand> command(new FetchPartCommand(*this, folderId, emailId, partId, priority));

	// Look for an active command
	MojRefCountedPtr<ImapCommand> activeCommand = FindActiveCommand(command);
	if(activeCommand.get()) {
		MojLogDebug(m_log, "attaching download request to existing active command");

		if(listener.get())
			static_cast<FetchPartCommand*>(activeCommand.get())->AddDownloadListener(listener);

		return;
	}

	// Look for a pending command with equal or higher priority
	// TODO: requeue if a higher priority is needed
	MojRefCountedPtr<ImapCommand> pendingCommand = FindPendingCommand(command);
	if(pendingCommand.get() && pendingCommand->GetPriority() >= priority) {
		MojLogDebug(m_log, "attaching download request to existing pending command");

		if(listener.get())
			static_cast<FetchPartCommand*>(pendingCommand.get())->AddDownloadListener(listener);

		// Poke the queue in case it's not already running
		CheckQueue();
		return;
	}

	// Otherwise, use the new command
	if(listener.get())
		command->AddDownloadListener(listener);

	// If this is a high priority request, set m_recentUserInteraction to true
	if (priority >= Command::HighPriority) {
		m_recentUserInteraction = true;
	}

	m_commandManager->QueueCommand(command, false);
	CheckQueue();
}

void ImapSession::SearchFolder(const MojObject& folderId, const MojRefCountedPtr<SearchRequest>& searchRequest)
{
	MojRefCountedPtr<SearchFolderCommand> command(new SearchFolderCommand(*this, folderId, searchRequest));

	m_commandManager->QueueCommand(command, false);
	CheckQueue();
}

void ImapSession::AutoDownloadParts(const MojObject& folderId)
{
	MojRefCountedPtr<AutoDownloadCommand> command(new AutoDownloadCommand(*this, folderId));

	m_commandManager->QueueCommand(command, false);
	CheckQueue();
}

void ImapSession::FetchNewHeaders(const MojObject& folderId)
{
	MojRefCountedPtr<FetchNewHeadersCommand> command(new FetchNewHeadersCommand(*this, folderId));

	m_commandManager->QueueCommand(command, false);
	CheckQueue();
}

void ImapSession::PrepareToConnect()
{
	SetState(State_QueryingNetworkStatus);

	m_reconnectRequested = false;
	m_sessionPushDuration = 0;

	if(m_client.get() && m_account.get())
		m_shouldPush = IsPushRequested(m_folderId);

	QueryNetworkStatus();
}

void ImapSession::QueryNetworkStatus()
{
	if(m_client.get() != NULL && !m_client->GetNetworkStatusMonitor().HasCurrentStatus()) {
		m_client->GetNetworkStatusMonitor().WaitForStatus(m_networkStatusSlot);
	} else {
		// If we already have the network status, or don't have a client, move on
		QueryNetworkStatusDone();
	}
}

MojErr ImapSession::NetworkStatusAvailable()
{
	QueryNetworkStatusDone();

	return MojErrNone;
}

void ImapSession::QueryNetworkStatusDone()
{
	bool hasConnection = false;

	if(m_client.get() && !ImapConfig::GetConfig().GetIgnoreNetworkStatus()) {
		if(m_client->GetNetworkStatusMonitor().HasCurrentStatus()) {
			const NetworkStatus& status = m_client->GetNetworkStatusMonitor().GetCurrentStatus();
			hasConnection = status.IsConnected();

			if(hasConnection && IsPushRequested(m_folderId)) {
				boost::shared_ptr<InterfaceStatus> interface = status.GetPersistentInterface();
				if(interface.get() != NULL) {
					MojLogDebug(m_log, "session %p setting bind address to %s", this, interface->GetIpAddress().c_str());
					m_connectBindAddress = interface->GetIpAddress();
				}
			}
		} else {
			// Unknown status; go ahead and try to connect
			MojLogDebug(m_log, "session %p network status unknown", this);
			hasConnection = true;
		}
	} else {
		// Ignore network status during validation
		hasConnection = true;
	}

	if(hasConnection) {
		SetState(State_Connecting);

		Connect();
	} else {
		MojLogWarning(m_log, "session %p no network connection available", this);
		Disconnected();
	}
}

void ImapSession::Connect()
{
	MojRefCountedPtr<ConnectCommand> command(new ConnectCommand(*this, m_connectBindAddress));

	// Clear bind address so it doesn't get reused unless we get another update
	// This way, we won't get stuck with a possibly invalid bind address.
	m_connectBindAddress.clear();

	m_stats.connectAttemptCount++;

	m_commandManager->RunCommand(command);
}

void ImapSession::ConnectSuccess()
{
}

bool ImapSession::ShouldReportError(MailError::ErrorCode errorCode) const
{
	switch(errorCode) {
	// account errors
	case MailError::BAD_USERNAME_OR_PASSWORD:
	case MailError::ACCOUNT_WEB_LOGIN_REQUIRED:
	case MailError::ACCOUNT_UNKNOWN_AUTH_ERROR:
	case MailError::ACCOUNT_LOCKED:
	case MailError::IMAP_NOT_ENABLED:
	// ssl errors
	case MailError::SSL_CERTIFICATE_EXPIRED:
	case MailError::SSL_CERTIFICATE_NOT_TRUSTED:
	case MailError::SSL_CERTIFICATE_INVALID:
	case MailError::SSL_HOST_NAME_MISMATCHED:
	// configuration errors
	case MailError::CONFIG_NEEDS_SSL:
	case MailError::CONFIG_NO_SSL:
	// internal error
	case MailError::INTERNAL_ERROR:
		return true;
	default:
		return false;
	}
}

void ImapSession::ConnectFailure(const exception& exc)
{
	MailError::ErrorInfo errorInfo = ExceptionUtils::GetErrorInfo(exc);

	if(ShouldReportError(errorInfo.errorCode))
		ReportStatus(errorInfo.errorCode);

	Disconnected();
}

void ImapSession::TLSFailure(const exception& exc)
{
	MailError::ErrorInfo errorInfo = ExceptionUtils::GetErrorInfo(exc);

	if(ShouldReportError(errorInfo.errorCode))
		ReportStatus(errorInfo.errorCode);

	Disconnect();
}

void ImapSession::LoginSuccess()
{
	m_stats.loginSuccessCount++;

	if (m_client.get()) {
		m_client->LoginSuccess();

		// Clear existing account error, if there is one
		if(m_account.get() && m_account->GetError().errorCode != MailError::NONE) {
			ReportStatus(MailError::NONE);
		}
	}

	if(m_state == State_PendingLogin) {

		// IMAP requires refetching the capability list after logging in.
		// The LOGIN response may have already updated the capabilities;
		// if not, we need to ask for them.

		// FIXME capabilities should be set invalid when logging in or enabling TLS
		if(m_capabilities.IsValid()) {
			CapabilityComplete();
		} else {
			QueryCapabilities();
		}

	} else {
		MojLogError(m_log, "%s called in wrong state %d", __PRETTY_FUNCTION__, m_state);
	}
}

void ImapSession::LoginFailure(MailError::ErrorCode errorCode, const std::string& serverMessage)
{
	MojLogError(m_log, "session %p: login failure: error %d: %s", this, errorCode, serverMessage.c_str());
	
	if(ShouldReportError(errorCode)) {
		ReportStatus( MailError::ErrorInfo(errorCode, serverMessage) );
	}
	
	Disconnect();
	// TODO: Remove this
}

void ImapSession::AuthYahooSuccess()
{
	LoginSuccess();
}

void ImapSession::AuthYahooFailure(MailError::ErrorCode errorCode, const std::string& errorText)
{
	LoginFailure(errorCode, errorText);
}

void ImapSession::QueryCapabilities()
{
	SetState(State_GettingCapabilities);

	MojRefCountedPtr<CapabilityCommand> command(new CapabilityCommand(*this));
	m_commandManager->RunCommand(command);
}

// This handles CAPABILITY both before and after login (as needed),
// or if no capability update was needed
void ImapSession::CapabilityComplete()
{
	if(m_state == State_GettingLoginCapabilities) {
		LoginCapabilityComplete();
	} else if(m_state == State_PendingLogin || m_state == State_GettingCapabilities) {
		// Check for compression support and whether it's enabled
		if(m_capabilities.HasCapability(Capabilities::COMPRESS_DEFLATE) && m_account->GetEnableCompression()) {
			RequestCompression();
		} else {
			SelectFolder();
		}
	}
}

void ImapSession::RequestCompression()
{
	SetState(State_RequestingCompression);

	MojRefCountedPtr<CompressCommand> command(new CompressCommand(*this));
	m_commandManager->RunCommand(command);
}

void ImapSession::CompressComplete(bool success)
{
	if(success) {
		m_inputStream.reset(new InflaterInputStream(m_inputStream, -15)); // This uses 44KB per connection
		m_outputStream.reset(new DeflaterOutputStream(m_outputStream, -10, 5)); // This uses 20KB per connection
		m_lineReader.reset(new LineReader(m_inputStream));
		m_compressionActive = true;
	}

	SelectFolder();
}

void ImapSession::SelectFolder()
{
	// Only sync folder lists if we're syncing the inbox
	if(m_account.get() == NULL || !IsValidId(m_folderId) || m_account->GetInboxFolderId() == m_folderId) {
		MojRefCountedPtr<SyncFolderListCommand> command(new SyncFolderListCommand(*this));

		if (!m_safeMode) {
			// Run command asynchronously
			m_commandManager->RunCommand(command);
		} else {
			// Workaround if running asynchronously failed for some reason (bad IMAP server?)
			m_commandManager->QueueCommand(command, false);
		}

		// This can be done asynchronously; move on to SelectFolder
	}

	SetState(State_SelectingFolder);

	MojRefCountedPtr<SelectFolderCommand> command(new SelectFolderCommand(*this, m_folderId));
	m_commandManager->RunCommand(command);
}

void ImapSession::IdleStarted(bool disconnectNow)
{
	if(m_state == State_PreparingToIdle) {
		SetState(State_Idling);
		m_idleStartTime = time(NULL);

		// Yahoo wants us to disconnect from the server while we're idling
		if(disconnectNow) {
			Disconnect();
		}

		// Check if any commands came in while we were starting idle
		if(m_commandManager->GetPendingCommandCount() > 0) {
			if(m_idleCommand.get()) {
				MojLogInfo(m_log, "ending idle early; we have work to do (pending command: %s)",
						m_commandManager->Top()->Describe().c_str());
				m_idleCommand->EndIdle();
				m_idleCommand.reset();
			}
		}
	}
}

void ImapSession::IdleError(const std::exception& e, bool fatal)
{
	MojLogError(m_log, "error while in idle: %s", e.what());

	if(m_state == State_Idling || m_state == State_PreparingToIdle) {
		SetState(State_Disconnecting);
		Disconnect();
	}
}

void ImapSession::IdleDone()
{
	int lastPushDuration = m_idleStartTime > 0 ? (time(NULL) - m_idleStartTime) : 0;
	m_sessionPushDuration += lastPushDuration;

	IdleMode previousIdleMode = m_idleMode;
	m_idleMode = IdleMode_None;

	// Reset time so we don't have stale values
	m_idleStartTime = 0;

	if(m_state == State_Idling && m_account->IsYahoo()) {
		// Need to reconnect to server after we finish idle
		RunState(State_NeedsConnection);
	} else if(m_state == State_Disconnecting && m_account->IsYahoo()) {
		// Finish cleaning up disconnected connection
		SetState(State_Cleanup);
		CleanupAfterDisconnect();
	} else if(m_state == State_Idling || m_state == State_PreparingToIdle) {
		// If m_state == State_PreparingToIdle && m_account->IsYahoo(), it's still connected to the server

		if(BetterInterfaceAvailable()) {
			MojLogInfo(m_log, "wifi network available; reconnecting on new interface");

			m_reconnectRequested = true;
			Logout();
		} else {
			RunState(State_OkToSync);
		}
	} else {
		// Something bad probably happened
		if(m_state == State_Disconnecting || m_state == State_Cleanup) {
			MojLogWarning(m_log, "push connection died after %d seconds (%.1f minutes total this session)", lastPushDuration, float(m_sessionPushDuration)/60);

			// For NOOP idle, it's OK if the NOOP fails since it just means we need to reconnect.
			if(previousIdleMode == IdleMode_NOOP) {
				m_reconnectRequested = true;
			}
		} else {
			MojLogWarning(m_log, "idle done in unexpected state %s (%d)", GetStateName(m_state), m_state);
		}
	}
}

// Cleanly log out
void ImapSession::Logout()
{
	if(m_connection.get() && ImapConfig::GetConfig().GetCleanDisconnect()) {
		// Send LOGOUT command to the server
		SetState(State_LoggingOut);

		MojRefCountedPtr<LogoutCommand> command(new LogoutCommand(*this));
		m_commandManager->RunCommand(command);
	} else {
		// Just close the connection
		SetState(State_Disconnecting);
		Disconnect();
	}
}

void ImapSession::LoggedOut()
{
	if(m_state == State_LoggingOut) {
		// Logout command acknowledged by server
		SetState(State_Disconnecting);
		Disconnect();
	} else {
		// Probably already disconnected by server
	}
}

const char* ImapSession::GetStateName(State state)
{
	switch(state) {
	case State_NeedsAccount: return "NeedsAccount";
	case State_NeedsConnection: return "NeedsConnection";
	case State_QueryingNetworkStatus: return "QueryingNetworkStatus";
	case State_Connecting: return "Connecting";
	case State_GettingLoginCapabilities: return "GettingLoginCapabilities";
	case State_NeedsTLS: return "NeedsTLS";
	case State_TLSNegotiation: return "TLSNegotiation";
	case State_LoginRequired: return "LoginRequired";
	case State_PendingLogin: return "PendingLogin";
	case State_GettingCapabilities: return "GettingCapabilities";
	case State_RequestingCompression: return "RequestingCompression";
	case State_SelectingFolder: return "SelectingFolder";
	case State_OkToSync: return "OkToSync";
	case State_PreparingToIdle: return "PreparingToIdle";
	case State_Idling: return "Idling";
	case State_LoggingOut: return "LoggingOut";
	case State_Disconnecting: return "Disconnecting";
	case State_Cleanup: return "Cleanup";
	case State_InvalidCredentials: return "InvalidCredentials";
	case State_AccountDeleted: return "AccountDeleted";
	}

	return "unknown";
}

void ImapSession::Status(MojObject& status)
{
	MojErr err;

	err = status.putString("sessionState", GetStateName(m_state));
	ErrorToException(err);

	MojObject timeStatus;
	err = timeStatus.put("enteredStateTime", (MojInt64) m_enteredStateTime);
	ErrorToException(err);
	err = timeStatus.put("secondsInState", (MojInt64) (time(NULL) - m_enteredStateTime));
	ErrorToException(err);

	err = status.put("time", timeStatus);
	ErrorToException(err);

	err = status.put("folderId", m_folderId);
	ErrorToException(err);

	MojObject commandManagerStatus;
	m_commandManager->Status(commandManagerStatus);
	err = status.put("sessionCommands", commandManagerStatus);
	ErrorToException(err);

	if(m_requestManager.get()) {
		MojObject requestManagerStatus;
		m_requestManager->Status(requestManagerStatus);
		err = status.put("requestManager", requestManagerStatus);
		ErrorToException(err);
	}

	if(m_lineReader.get()) {
		MojObject lineReaderStatus;
		m_lineReader->Status(lineReaderStatus);
		err = status.put("lineReader", lineReaderStatus);
		ErrorToException(err);
	}

	if(m_connection.get()) {
		MojObject connectionStatus;
		m_connection->Status(connectionStatus);

		if(m_compressionActive) {
			err = connectionStatus.put("compression", true);
			ErrorToException(err);
		}

		err = status.put("connection", connectionStatus);
		ErrorToException(err);
	}

	// Stats
	if(true) {
		MojObject stats;

		err = stats.put("connectAttemptCount", m_stats.connectAttemptCount);
		ErrorToException(err);

		err = stats.put("loginSuccessCount", m_stats.loginSuccessCount);
		ErrorToException(err);

		err = status.put("stats", stats);
		ErrorToException(err);
	}
}

void ImapSession::FatalError(const std::string& reason)
{
	if(IsConnected()) {
		MojLogInfo(m_log, "fatal error; disconnecting: %s", reason.c_str());

		SetState(State_Disconnecting);
		Disconnect();
	}
}

void ImapSession::Disconnect()
{
	MojLogInfo(m_log, "session %p disconnecting from server", this);

	if(m_connection.get()) {
		// Keep a local reference so it doesn't get deleted while in Shutdown
		// We clear the m_connection reference in ImapSession::ConnectionClosed
		MojRefCountedPtr<SocketConnection> tempConnection = m_connection;
		tempConnection->Shutdown();
	} else {
		ConnectionClosed();
	}
}

void ImapSession::RequestLine(const MojRefCountedPtr<ImapResponseParser>& responseParser, int timeoutSeconds)
{
	m_requestManager->RequestLine(responseParser, timeoutSeconds);
}

void ImapSession::RequestData(const MojRefCountedPtr<ImapResponseParser>& responseParser, size_t bytes, int timeoutSeconds)
{
	m_requestManager->RequestData(responseParser, bytes, timeoutSeconds);
}

void ImapSession::WaitForParser(const MojRefCountedPtr<ImapResponseParser>& responseParser)
{
	m_requestManager->WaitForParser(responseParser);
}

void ImapSession::ParserFinished(const MojRefCountedPtr<ImapResponseParser>& responseParser)
{
	m_requestManager->ParserFinished(responseParser);
}

MojErr ImapSession::ConnectionClosed()
{
	MojLogInfo(m_log, "session %p connection closed", this);

	// FIXME check other states (deleted, etc)
	if(m_state != State_NeedsConnection) {
		if(m_account->IsYahoo() && m_account->IsPush() && m_state == State_Idling) {
			// Yahoo is a special case: when it goes into idle, it disconnects from the server
			// In this case, we'll just delete the connection but stay in the idle state
			MojLogInfo(m_log, "Resetting connection object for Yahoo idle");
			ResetConnection();
		} else {
			// Clean up any incomplete Yahoo idle state
			if(m_account->IsYahoo() && m_idleCommand.get() && m_state == State_Disconnecting) {
				if(m_idleCommand.get()) {
					MojLogInfo(m_log, "ending idle; we are disconnecting");
					m_idleCommand->EndIdle();
				}
			} else {
				SetState(State_Disconnecting);
			}

			// If the state is still State_Disconnecting, check if all the commands are complete
			if(m_state == State_Disconnecting)
				CheckQueue();
		}
	}

	return MojErrNone;
}

// This gets called after the connection is lost AND all commands have failed
void ImapSession::Disconnected()
{
	ResetConnection();
	SetState(State_Cleanup);
	CleanupAfterDisconnect();
}

void ImapSession::CollectConnectionStats(CompressionStats& stats)
{
	// Collect stats
	if(m_compressionActive) {
		InflaterInputStream* iis = dynamic_cast<InflaterInputStream*>(m_inputStream.get());

		if(iis != NULL) {
			InflaterInputStream::InflateStats inflateStats;
			iis->GetInflateStats(inflateStats);

			stats.totalBytesIn += inflateStats.bytesOut;
			stats.compressedBytesIn += inflateStats.bytesIn;
		}

		DeflaterOutputStream* dis = dynamic_cast<DeflaterOutputStream*>(m_outputStream.get());

		if(dis != NULL) {
			DeflaterOutputStream::DeflateStats deflateStats;
			dis->GetDeflateStats(deflateStats);

			stats.totalBytesOut += deflateStats.bytesIn;
			stats.compressedBytesOut += deflateStats.bytesOut;
		}
	}
}

void ImapSession::ResetConnection()
{
	CompressionStats stats;
	CollectConnectionStats(stats);

	if(m_compressionActive) {
		MojInt64 total = stats.totalBytesIn + stats.totalBytesOut;
		MojInt64 compressed = stats.compressedBytesIn + stats.compressedBytesOut;

		MojLogInfo(m_log, "saved %.1f/%.1f KB from compression this session",
				float(total - compressed)/1024,
				float(total)/1024);
	}

	m_stats.compressionStats += stats;

	// Schedule an async cleanup so this doesn't get cleaned up immediately
	if (m_requestManager.get()) {
		s_asyncCleanupItems.push_back(m_requestManager);
		ScheduleAsyncCleanup();
	}

	m_connection.reset();
	m_inputStream.reset();
	m_outputStream.reset();
	m_lineReader.reset();

	m_folderSession.reset();
	m_requestManager.reset(new ImapRequestManager(*this));

	m_compressionActive = false;
}

// Cleanup sync sessions and activities
// Part of State_Cleanup
void ImapSession::CleanupAfterDisconnect()
{
	if(m_client.get() && !m_client->DisableAccountInProgress()) {
		MojRefCountedPtr<SyncSession> syncSession = m_client->GetSyncSession(m_folderId);

		if(syncSession.get()) {
			MojLogInfo(m_log, "stopping sync session after disconnect");
			// Indicate that the sync session may need to schedule a retry.
			// This will probably get ignored if the sync session is already ending (successful or not).
			syncSession->SetQueueStopped();

			// Stop the sync session
			syncSession->RequestStop(m_syncSessionDoneSlot);
		} else {
			// Schedule push, if necessary
			SchedulePush();
		}
	} else {
		EndActivities();
	}
}

// Part of State_Cleanup
MojErr ImapSession::SyncSessionDone()
{
	try {
		SchedulePush();
	} catch(const std::exception& e) {
		MojLogCritical(m_log, "exception in %s:%d: %s", __FILE__, __LINE__, e.what());
	}

	return MojErrNone;
}

void ImapSession::SchedulePush()
{
	if(m_shouldPush) {
		if(m_sessionPushDuration > 5 * 60) {
			// If we were able to idle for at least 5 minutes total, ignore any retry count.
			m_pushRetryCount = 0;
		}

		// If the connection is stable, and we didn't already retry, reconnect.
		if (m_pushRetryCount < 1 && CheckNetworkHealthForPush()) {
			MojLogInfo(m_log, "first push connect retry; reconnecting immediately");
			m_reconnectRequested = true;

			m_pushRetryCount++;
		}
	}

	if(m_shouldPush && !m_reconnectRequested) {
		MojLogNotice(m_log, "scheduling retry after disconnect since push is enabled");
		// Schedule retry if necessary
		// TODO: if we can tell that the interface went down, we can just reschedule the idle activity instead of a full sync.
		SyncParams syncParams;

		if(m_pushRetryCount > 0) {
			// FIXME hack so it goes to the next retry interval when scheduling push
			EmailAccount::RetryStatus retryStatus;
			retryStatus.SetReason("retry push");
			retryStatus.SetInterval(60);
			m_client->GetAccount().SetRetry(retryStatus);
		}

		// Reset retry count
		m_pushRetryCount = 0;

		// FIXME don't reschedule if the sync session already scheduled a retry
		m_scheduleRetryCommand.reset(new ScheduleRetryCommand(*m_client, m_folderId, syncParams));
		m_scheduleRetryCommand->Run(m_scheduleRetrySlot);
	} else {
		EndActivities();
	}
}

// Part of State_Cleanup
MojErr ImapSession::ScheduleRetryDone()
{
	try {
		EndActivities();
	} catch(const std::exception& e) {
		MojLogCritical(m_log, "exception in %s:%d: %s", __FILE__, __LINE__, e.what());
	}

	return MojErrNone;
}

// This gets called after all sync sessions have shut down
// Part of State_Cleanup
void ImapSession::EndActivities()
{
	if(m_activitySet.get()) {
		m_activitySet->EndActivities(m_activitiesEndedSlot);
	} else {
		ActivitiesEnded();
	}
}

// Part of State_Cleanup
MojErr ImapSession::ActivitiesEnded()
{
	CleanupDone();
	return MojErrNone;
}

// Part of State_Cleanup
void ImapSession::CleanupDone()
{
	MojLogInfo(m_log, "disconnect cleanup complete");

	m_powerUser.Stop();

	// Clear capabilities
	m_capabilities.Clear();

	bool reconnectNow = false;

	if(m_reconnectRequested) {
		m_reconnectRequested = false;

		// Connect to the server and run any pending commands
		if(m_commandManager->GetPendingCommandCount() > 0 || IsPushRequested(m_folderId)) {
			reconnectNow = true;
		}
	}

	if(reconnectNow && (!HasClient() || !GetClient()->DisableAccountInProgress())) {
		PrepareToConnect();
	} else {
		// FIXME check for invalid login state
		SetState(State_NeedsConnection);

		if(m_client.get()) {
			m_client->SessionDisconnected(m_folderId);
		}

		// Not going to retry right now; cancel pending commands
		CancelPendingCommands(ImapCommand::CancelType_NoConnection);
	}
}

bool ImapSession::IsPushRequested(const MojObject& folderId)
{
	return m_account->IsPush() && folderId == m_account->GetInboxFolderId();
}

bool ImapSession::IsPushAvailable(const MojObject& folderId)
{
	return m_account->IsYahoo() || GetCapabilities().HasCapability(Capabilities::IDLE);
}

bool ImapSession::IsPushEnabled(const MojObject& folderId)
{
	return IsPushRequested(folderId) && IsPushAvailable(folderId);
}

bool ImapSession::IsIdling() const
{
	return m_state == State_Idling;
}

void ImapSession::WakeupIdle()
{
	if(m_state == State_Idling) {
		if(m_idleCommand.get()) {
			MojLogInfo(m_log, "ending idle; wakeup called");
			m_idleCommand->EndIdle();
			m_idleCommand.reset();
		}
	}
}

void ImapSession::ReportStatus(const MailError::ErrorInfo& errorInfo)
{
	if(m_client.get()) {
		m_client->UpdateAccountStatus(errorInfo);
	}
}

const MojRefCountedPtr<ActivitySet>& ImapSession::GetActivitySet()
{
	if(m_activitySet.get() == NULL) {
		m_activitySet.reset(new ActivitySet(GetBusClient()));
	}

	return m_activitySet;
}

void ImapSession::AddActivities(const std::vector<ActivityPtr>& activities)
{
	BOOST_FOREACH(const ActivityPtr& activity, activities) {
		activity->SetEndAction(Activity::EndAction_Complete);
		GetActivitySet()->AddActivity(activity);
	}
}

void ImapSession::ForceReconnect(const std::string& reason)
{
	m_reconnectRequested = true;

	if(m_state == State_OkToSync) {
		MojLogInfo(m_log, "terminating connection: %s", reason.c_str());
		SetState(State_Disconnecting);
		Disconnect();
	}
}

bool ImapSession::BetterInterfaceAvailable()
{
	string currentBindAddress;

	if(m_connection.get()) {
		currentBindAddress = m_connection->GetBindAddress();

		//MojLogDebug(m_log, "current interface: %s", currentBindAddress.c_str());
	}

	if(m_client.get() && !currentBindAddress.empty()) {
		NetworkStatusMonitor& monitor = m_client->GetNetworkStatusMonitor();

		if(monitor.HasCurrentStatus()) {
			const NetworkStatus& networkStatus = monitor.GetCurrentStatus();

			boost::shared_ptr<InterfaceStatus> wanStatus = networkStatus.GetWanStatus();
			boost::shared_ptr<InterfaceStatus> wifiStatus = networkStatus.GetWifiStatus();

			// Check if Wifi is available and in good health
			if(wifiStatus.get() && wifiStatus->IsWakeOnWifiEnabled()
					&& wifiStatus->GetNetworkConfidence() >= InterfaceStatus::EXCELLENT) {

				MojLogDebug(m_log, "wifi is excellent");

				// Check if we're currently on WAN
				if(wanStatus.get() && wanStatus->GetIpAddress() == currentBindAddress) {
					// We should switch to Wifi
					return true;
				}
			}

		}
	}

	return false;
}

void ImapSession::CancelPendingCommands(ImapCommand::CancelType cancelType)
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

void ImapSession::ScheduleAsyncCleanup()
{
	if (!s_asyncCleanupCallbackId) {
		s_asyncCleanupCallbackId = g_idle_add(&ImapSession::AsyncCleanup, NULL);
	}
}

gboolean ImapSession::AsyncCleanup(gpointer data)
{
	s_asyncCleanupCallbackId = 0;
	s_asyncCleanupItems.clear();

	//fprintf(stderr, "async cleanup\n");

	return false;
}
