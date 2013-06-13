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

#ifndef IMAPSESSION_H_
#define IMAPSESSION_H_

#include "core/MojSignal.h"
#include "core/MojObject.h"
#include "network/SocketConnection.h"
#include <string>
#include <vector>
#include "client/Command.h"
#include "client/CommandManager.h"
#include "stream/LineReader.h"
#include "client/ImapRequestManager.h"
#include "CommonErrors.h"
#include "ImapClient.h"
#include "client/Capabilities.h"
#include "CommonErrors.h"
#include "client/PowerUser.h"
#include "client/SyncParams.h"
#include "data/ImapAccount.h"
#include <ctime>

class ImapClient;
class ImapResponseParser;
class DatabaseInterface;
class FolderSession;

class BusClient;
class FileCacheClient;
class DownloadListener;

class BaseIdleCommand;
class ScheduleRetryCommand;

class ImapLoginSettings;

class ActivitySet;

class ImapSession : public MojSignalHandler, public Command::Listener
{
	static const int DEFAULT_REQUEST_TIMEOUT;
	static const int FALLBACK_SYNC_INTERVAL;
	static const int STATE_POWER_TIMEOUT;
	static const int SYNC_STATE_POWER_TIMEOUT;

public:
	ImapSession(const MojRefCountedPtr<ImapClient>& client, MojLogger& logger = ImapSession::s_log);
	virtual ~ImapSession();
	
	void SetDatabase(DatabaseInterface& dbInterface);
	void SetFileCacheClient(FileCacheClient& fileCacheClient);
	void SetBusClient(BusClient& fileCacheClient);
	void SetPowerManager(const MojRefCountedPtr<PowerManager>& powerManager);

	/*
	 * List of ImapSession states. If you modify this, make sure the following functions are also updated:
	 *
	 * CheckQueue()
	 * NeedsPowerInState()
	 * IsConnected()
	 * IsActive()
	 *
	 */
	typedef enum {
		State_NeedsAccount,
		State_NeedsConnection,
		State_QueryingNetworkStatus,
		State_Connecting,
		State_GettingLoginCapabilities,	// detect STARTTLS, LOGINDISABLED
		State_NeedsTLS,
		State_TLSNegotiation,
		State_LoginRequired,
		State_PendingLogin,
		State_GettingCapabilities,
		State_RequestingCompression,
		State_SelectingFolder,
		State_OkToSync,
		State_PreparingToIdle,		// starting to issue idle command
		State_Idling,
		State_LoggingOut,			// (optional) logout stage; disabled by default
		State_Disconnecting,		// waiting for commands to fail
		State_Cleanup,				// waiting for sync sessions to finish
		State_InvalidCredentials,
		State_AccountDeleted
	} State;

	/**
	 * Sends a request to the server.
	 * A tag will be prepended automatically to track the response.
	 * 
	 * @return generated tag
	 */
	void SendRequest(const std::string& request, MojRefCountedPtr<ImapResponseParser> responseParser, int timeout = DEFAULT_REQUEST_TIMEOUT, bool canLogRequest = true);

	/**
	 * Update the timeout for a given parser.
	 */
	void UpdateRequestTimeout(const MojRefCountedPtr<ImapResponseParser>& responseParser, int timeoutSeconds);

	// Get connection input stream. Throws exception if not connected to server.
	InputStreamPtr&		GetInputStream();

	// Get output stream. Throws exception if not connected to server.
	OutputStreamPtr&	GetOutputStream();

	// Get the line reader for the current connection. Throws exception if no connection is available.
	LineReaderPtr&		GetLineReader();

	// Get the connection for this session. Does *not* throw an exception if there's no connection.
	const MojRefCountedPtr<SocketConnection>& GetConnection();
	
	MojLogger&			GetLogger() { return m_log; }
	
	const MojRefCountedPtr<ImapClient>&	GetClient() const { return m_client; }
	bool HasClient() const { return m_client.get(); }

	virtual void CommandComplete(Command* command);
	virtual void CommandFailure(Command* command, const std::exception& e);

	const boost::shared_ptr<ImapAccount>& GetAccount();
	
	void SetAccount(boost::shared_ptr<ImapAccount>& toSet);
	void SetFolder(const MojObject& folderId);
	
	// Functions for queueing commands
	virtual void AutoDownloadParts(const MojObject& folderId);
	virtual void SyncAccount();
	virtual void SyncFolder(const MojObject& folderId, SyncParams params);
	virtual void UpSyncSentEmails(const MojObject& folderId, const ActivityPtr& activity);
	virtual void UpSyncDrafts(const MojObject& folderId, const ActivityPtr& activity);
	virtual void DownloadPart(const MojObject& folderId, const MojObject& emailId, const MojObject& partId,
			const MojRefCountedPtr<DownloadListener>& listener, Command::Priority priority);
	virtual void SearchFolder(const MojObject& folderId, const MojRefCountedPtr<SearchRequest>& searchRequest);
	virtual void FetchNewHeaders(const MojObject& folderId);

	virtual void Status(MojObject& status);

	// Disconnect from server
	virtual void Disconnect();
	virtual void Disconnect(State newState) { m_state = newState; Disconnect(); }

	// Used to request additional data, which pauses reading command responses in ImapRequestManager.
	virtual void RequestLine(const MojRefCountedPtr<ImapResponseParser>& responseParser, int timeoutSeconds = 0);
	virtual void RequestData(const MojRefCountedPtr<ImapResponseParser>& responseParser, size_t bytes, int timeoutSeconds = 0);

	// TODO: make WaitForParser private?
	virtual void WaitForParser(const MojRefCountedPtr<ImapResponseParser>& responseParser);

	/**
	 * Returns control of the connection stream to the request manager.
	 */
	virtual void ParserFinished(const MojRefCountedPtr<ImapResponseParser>& responseParser);

	// These methods are called from commands to update the session state.
	virtual void PrepareToConnect();
	virtual void QueryNetworkStatus();
	virtual MojErr NetworkStatusAvailable();
	virtual void QueryNetworkStatusDone();

	virtual void Connect();
	virtual void ConnectSuccess();
	virtual void ConnectFailure(const std::exception& e);

	virtual void Connected(const MojRefCountedPtr<SocketConnection>& connection);

	virtual void CheckLoginCapabilities();
	virtual void LoginCapabilityComplete();

	virtual void TLSReady();
	virtual void TLSFailure(const std::exception& e);

	virtual void LoginSuccess();
	virtual void LoginFailure(MailError::ErrorCode errorCode, const std::string& serverMessage = "");

	virtual void AuthYahooSuccess();
	virtual void AuthYahooFailure(MailError::ErrorCode errorCode, const std::string& errorText = "");

	// Called from CapabilityCommand
	virtual void CapabilityComplete();

	virtual void RequestCompression();
	virtual void CompressComplete(bool success);

	// Select folder; may also asynchronously sync the folder list
	virtual void SelectFolder();

	virtual void FolderListSynced();

	virtual void FolderSelected();
	virtual void FolderSelectFailed();
	
	// Called from IdleCommand to indicate that we're starting idle
	virtual void IdleStarted(bool disconnectNow=false);

	// Called from IdleCommand to indicate that we're done idling
	virtual void IdleError(const std::exception& e, bool fatal);
	virtual void IdleDone();

	virtual void DisconnectSession();

	// Wakeup out of idle to run a command or refresh the idle.
	virtual void WakeupIdle();

	// Add activities
	virtual void AddActivities(const std::vector<ActivityPtr>& activities);

	virtual const MojRefCountedPtr<ActivitySet>& GetActivitySet();

	virtual BusClient& GetBusClient()
	{
		if(m_busClient)
			return *m_busClient;
		else
			throw MailException("no bus client configured", __FILE__, __LINE__);
	}

	DatabaseInterface& GetDatabaseInterface() const
	{
		if(m_dbInterface)
			return *m_dbInterface;
		else
			throw MailException("no database interface configured", __FILE__, __LINE__);
	}
	
	FileCacheClient& GetFileCacheClient() const
	{
		if(m_fileCacheClient)
			return *m_fileCacheClient;
		else
			throw MailException("no file cache client configured", __FILE__, __LINE__);
	}

	bool HasPowerManager() const
	{
		return m_powerManager.get() != NULL;
	}

	const MojRefCountedPtr<PowerManager>& GetPowerManager() const
	{
		if(m_powerManager.get())
			return m_powerManager;
		else
			throw MailException("no power manager configured", __FILE__, __LINE__);
	}

	virtual const ImapLoginSettings& GetLoginSettings()
	{
		return GetAccount()->GetLoginSettings();
	}

	const boost::shared_ptr<FolderSession>& GetFolderSession() const { return m_folderSession; }
	void SetFolderSession(const boost::shared_ptr<FolderSession>& folderSession) { m_folderSession = folderSession; }

	Capabilities&		GetCapabilities() { return m_capabilities; }

	bool IsPushEnabled(const MojObject& folderId);
	bool IsPushAvailable(const MojObject& folderId);
	bool IsPushRequested(const MojObject& folderId);

	// Are we currently connected, or in the process of connecting?
	bool IsConnected() const;

	// Returns true if the session has something to do
	bool IsActive() const;

	// Return true if the session is currently in idling state
	bool IsIdling() const;

	// Kills the connection due to an unrecoverable error (usually in the parser),
	// where we don't know the state of the connection.
	void FatalError(const std::string& e);

	std::string Describe() const;

	bool IsSafeMode() const { return m_safeMode; }
	void SetSafeMode(bool enabled) { m_safeMode = enabled; }

	void ForceReconnect(const std::string& reason);

	virtual bool IsValidator() const { return false; }

	// Cleanly log out and disconnect
	void Logout();
	void LoggedOut();

protected:
	void SetState(State newState);
	void RunState(State newState);
	void StateTimeout();

	void RunCommandsInQueue();

	void CheckQueue();

	bool ShouldReportError(MailError::ErrorCode errCode) const;

	void ReportStatus(const MailError::ErrorInfo& errorInfo);

	bool NeedsPowerInState(State state) const;

	virtual bool IsYahoo() const { return m_account->IsYahoo(); }

	// Indicate that the user has interacted with the session recently, and may do so again soon
	virtual void UpdateLastInteraction() { m_recentUserInteraction = true; }

	// This gets called after the connection is lost and all commands have failed
	virtual void Disconnected();

	virtual void ResetConnection();

	virtual void CleanupAfterDisconnect();

	// This gets called when the sync session is done cleaning up
	virtual MojErr SyncSessionDone();

	// Schedule a retry for the push
	void SchedulePush();

	// This gets called when the sync session is done cleaning up
	virtual MojErr ScheduleRetryDone();

	// End activities after disconnect
	virtual void EndActivities();

	// All activities ended
	virtual MojErr ActivitiesEnded();

	// This gets called when sync sessions have been completed
	virtual void CleanupDone();

	// Called when the connection is closed (on purpose or otherwise)
	virtual MojErr ConnectionClosed();

	// Request capabilities after logging in
	virtual void QueryCapabilities();

	virtual bool NeedLoginCapabilities();

	static const char* GetStateName(State state);

	int GetNoopIdleTimeout();

	// Find an existing command that matches the one passed in
	MojRefCountedPtr<ImapCommand> FindActiveCommand(const MojRefCountedPtr<ImapCommand>& command);
	MojRefCountedPtr<ImapCommand> FindPendingCommand(const MojRefCountedPtr<ImapCommand>& command);

	void CancelPendingCommands(ImapCommand::CancelType cancelType);

	struct CompressionStats
	{
		CompressionStats() : totalBytesIn(0), totalBytesOut(0), compressedBytesIn(0), compressedBytesOut(0) {}

		CompressionStats& operator +=(const CompressionStats& other) {
			totalBytesIn += other.totalBytesIn;
			totalBytesOut += other.totalBytesOut;
			compressedBytesIn += other.compressedBytesIn;
			compressedBytesOut += other.compressedBytesOut;
			return *this;
		}

		MojInt64	totalBytesIn;
		MojInt64	totalBytesOut;
		MojInt64	compressedBytesIn;
		MojInt64	compressedBytesOut;
	};

	struct Stats
	{
		Stats() : connectAttemptCount(0), loginSuccessCount(0) {}

		int					connectAttemptCount;
		int					loginSuccessCount;

		CompressionStats	compressionStats;
	};

	enum IdleMode
	{
		IdleMode_None,
		IdleMode_YahooPush,
		IdleMode_IDLE,
		IdleMode_NOOP
	};

	void CompressionStatus(MojObject& status, CompressionStats& stats);
	virtual void CollectConnectionStats(CompressionStats& status);

	bool BetterInterfaceAvailable();

	bool CheckNetworkHealthForPush();

	// Used for cleaning up after the stack is unwound
	static void ScheduleAsyncCleanup();
	static gboolean AsyncCleanup(gpointer data);

	static std::vector< MojRefCountedPtr<MojRefCounted> >	s_asyncCleanupItems;
	static guint							s_asyncCleanupCallbackId;

	MojLogger&								m_log;
	static MojLogger						s_log;
	
	DatabaseInterface*						m_dbInterface;
	FileCacheClient*						m_fileCacheClient;
	BusClient*								m_busClient;
	MojRefCountedPtr<PowerManager>			m_powerManager;

	MojRefCountedPtr<CommandManager>		m_commandManager;
	MojRefCountedPtr<ImapRequestManager>	m_requestManager;
	
	MojRefCountedPtr<SocketConnection>		m_connection;
	boost::shared_ptr<ImapAccount> 			m_account;

	State									m_state;
	InputStreamPtr							m_inputStream;
	OutputStreamPtr							m_outputStream;
	
	LineReaderPtr							m_lineReader;
	MojRefCountedPtr<ImapClient>			m_client;
	MojObject 								m_folderId;

	Capabilities							m_capabilities;

	boost::shared_ptr<FolderSession>		m_folderSession;

	MojRefCountedPtr<BaseIdleCommand>		m_idleCommand;
	MojRefCountedPtr<ScheduleRetryCommand>	m_scheduleRetryCommand;

	// This manages the ImapSession-specific (per-connection) activities.
	// Most sync-related activities go in SyncSession instead.
	MojRefCountedPtr<ActivitySet>			m_activitySet;

	PowerUser								m_powerUser;

	bool									m_recentUserInteraction;
	bool									m_reconnectRequested;

	// If Safe Mode is enabled, we'll try to be conservative like parsing
	// only one email at a time, disable compression, etc.
	bool									m_safeMode;

	// Whether compression is currently enabled
	bool									m_compressionActive;

	// Whether we want to push
	bool									m_shouldPush;

	// Idle mode
	IdleMode								m_idleMode;

	// How many times we've attempted to idle
	int										m_pushRetryCount;

	// Timestamp when we entered the current state
	time_t									m_enteredStateTime;
	time_t									m_idleStartTime;

	// Total time we've been able to idle/push since connecting
	int										m_sessionPushDuration;

	// Useful stats
	Stats									m_stats;

	// Timer to unlock power activity if it gets stuck
	Timer<ImapSession>						m_stateTimer;

	// Bind address to connect to on next connect.
	// Will get cleared after attempting to connect so we don't use stale data.
	// (Otherwise, an invalid bind address could get this stuck in a bad state.)
	std::string								m_connectBindAddress;

	MojSignal<>::Slot<ImapSession>			m_networkStatusSlot;
	MojSignal<>::Slot<ImapSession>			m_closedSlot;
	MojSignal<>::Slot<ImapSession>			m_syncSessionDoneSlot;
	MojSignal<>::Slot<ImapSession>			m_scheduleRetrySlot;
	MojSignal<>::Slot<ImapSession>			m_activitiesEndedSlot;
};

#endif /*IMAPSESSION_H_*/
