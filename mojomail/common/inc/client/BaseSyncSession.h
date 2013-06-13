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

#ifndef BASESYNCSESSION_H_
#define BASESYNCSESSION_H_

#include "core/MojSignal.h"
#include "core/MojObject.h"
#include "activity/Activity.h"
#include "activity/ActivitySet.h"
#include "db/MojDbClient.h"
#include <vector>
#include <set>
#include <map>
#include "data/SyncState.h"
#include "CommonErrors.h"

class ActivityBuilder;
class Command;
class SyncStateUpdater;

class BaseSyncSession : public MojSignalHandler
{
protected:
	enum SyncSessionState {
		State_None,
		State_Starting,
		State_Adopting,
		State_Active,
		State_Ending
	};

	typedef MojRefCountedPtr<Command> CommandPtr;

public:
	BaseSyncSession(BusClient& busClient, MojLogger& logger, const MojObject& accountId, const MojObject& folderId);
	virtual ~BaseSyncSession();

	// Starts a sync session if one hasn't already been started
	virtual void	RequestStart();

	/**
	 * Stop a sync session (e.g. connection lost).
	 * This may only be called if no commands are actively running.
	 *
	 * Any queued commands will be removed from the sync session.
	 */
	virtual void	RequestStop();
	virtual void	RequestStop(MojSignal<>::SlotRef stoppedSlot);

	virtual bool	IsActive() const;
	virtual bool	IsStarting() const;
	virtual bool	IsEnding() const;
	virtual bool	IsRunning() const;
	virtual bool	IsFinished() const;

	virtual void	AdoptActivity(const ActivityPtr& activity);

	virtual void	RegisterCommand(Command* command);
	virtual void	RegisterCommand(Command* command, MojSignal<>::SlotRef readySlot);

	virtual void	CommandStarted(Command* command);
	virtual void	CommandFailed(Command* command, const std::exception& e);
	virtual void	CommandCompleted(Command* command);

	// Get the last rev synced to the server by the folder
	MojInt64 GetLastSyncRev() const;

	// Add a revision to list of known new revs
	inline void AddRev(MojInt64 rev) { m_knownNewRevs.insert(rev); }

	// Extract revision list from database put response and add it
	// to the list of known new revs.
	void AddPutResponseRevs(const MojObject& response);

	// Set the highest rev that has been sync'd to the server.
	void SetNextSyncRev(MojInt64 rev);

	virtual void Status(MojObject& status) const;

protected:
	void SetState(SyncSessionState state);
	virtual void Start();
	virtual void End();

	// Returns true if command is ready to run
	virtual bool AddCommand(Command* command);

	// State_Starting
	virtual void	GetFolderData();
	virtual MojErr	GetFolderResponse(MojObject& response, MojErr err);

	virtual void	StartSpinner();
	virtual void	StartAccountSpinner();
	virtual MojErr	StartAccountSpinnerResponse();
	virtual void	StartFolderSpinner();
	virtual MojErr	StartFolderSpinnerResponse();
	virtual void	StartSpinnerDone();

	virtual void	AdoptActivities();
	virtual MojErr	ActivitiesAdopted();

	virtual void	UpdateFolder();
	virtual MojErr	UpdateFolderResponse(MojObject& response, MojErr err);

	// Update activity objects. Changes will be committed in EndActivities
	virtual void	UpdateAndEndActivities();
	virtual void	UpdateActivities();

	virtual void	StopSpinner();
	virtual void	StopAccountSpinner();
	virtual MojErr	StopAccountSpinnerResponse();
	virtual void	StopFolderSpinner();
	virtual MojErr	StopFolderSpinnerResponse();
	virtual void	StopSpinnerDone();

	virtual void	EndActivities();
	virtual MojErr	ActivitiesEnded();

	virtual void	GetNewChanges();
	virtual MojErr	GetNewChangesResponse(MojObject& response, MojErr err);

	// Internal error handling
	virtual void Cleanup();
	virtual void HandleError(const std::exception& e, const char* file, int line);
	virtual void HandleError(const char* file, int line);

	// May be overridden by subclasses provided they call the superclass implementation afterwards
	virtual void SyncSessionReady();
	virtual void SyncSessionComplete();

	// Get current sync state
	virtual SyncState GetFolderSyncState();
	virtual SyncState GetAccountSyncState();

	virtual void BuildFolderStatus(MojObject& folderStatus);

	// Is this an account sync (i.e. are we syncing the inbox?)
	virtual bool IsAccountSync();

	// Database adapters
	virtual void GetNewChanges(MojDbClient::Signal::SlotRef slot, MojObject folderId, MojInt64 rev, MojDbQuery::Page &page) = 0;
	virtual void GetById(MojDbClient::Signal::SlotRef slot, const MojObject& id) = 0;
	virtual void Merge(MojDbClient::Signal::SlotRef slot, const MojObject& obj) = 0;
	virtual MojString GetCapabilityProvider() = 0;
	virtual MojString GetBusAddress() = 0;

	static const char* GetStateName(SyncSessionState state);

	SyncSessionState	m_state;
	BusClient&	m_busClient;
	MojLogger&	m_log;
	MojObject	m_accountId;
	MojObject	m_folderId;

	std::string	m_syncSessionName;

	bool		m_foundFolder;

	// Shutting down connection
	bool		m_stopRequested;

	// A pending command is stuck waiting; need to restart the sync session
	bool		m_startRequested;

	// First sync of the folder
	bool		m_initialSync;

	// Starting revision. We should sync changes higher than this rev.
	MojInt64		m_lastSyncRev;

	// This value is updated with the highest revision that has been sync'd to the server.
	// It doesn't include new changes that were generated as a result of the sync.
	MojInt64		m_nextSyncRev;

	MojRefCountedPtr<ActivitySet>		m_activities;
	MojRefCountedPtr<ActivitySet>		m_queuedActivities;

	// Commands that have been added to the session but not run yet
	std::vector<CommandPtr>		m_waitingCommands;

	// Commands that are currently running and part of the session
	std::vector<CommandPtr>		m_readyCommands;

	// Commands that recently failed
	std::map<CommandPtr, boost::exception_ptr>		m_failedCommands;

	MailError::ErrorInfo	m_lastError;

	// List of revisions triggered by the transport updating the database.
	std::set<MojInt64>	m_knownNewRevs;
	MojDbQuery::Page	m_changesPage;

	MojRefCountedPtr<SyncStateUpdater>	m_syncStateUpdater;

	MojSignal<>			m_readySignal;
	MojSignal<>			m_stoppedSignal;

	MojDbClient::Signal::Slot<BaseSyncSession>			m_getFolderSlot;
	MojSignal<>::Slot<BaseSyncSession>					m_startAccountSpinnerSlot;
	MojSignal<>::Slot<BaseSyncSession>					m_startFolderSpinnerSlot;
	MojSignal<>::Slot<BaseSyncSession>					m_adoptActivitiesSlot;
	MojDbClient::Signal::Slot<BaseSyncSession>			m_getNewChangesSlot;
	MojSignal<>::Slot<BaseSyncSession>					m_endActivitiesSlot;
	MojDbClient::Signal::Slot<BaseSyncSession>			m_updateFolderSlot;
	MojSignal<>::Slot<BaseSyncSession>					m_stopAccountSpinnerSlot;
	MojSignal<>::Slot<BaseSyncSession>					m_stopFolderSpinnerSlot;
};

#endif /* BASESYNCSESSION_H_ */
