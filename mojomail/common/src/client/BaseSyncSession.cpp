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

#include "client/BaseSyncSession.h"
#include "activity/ActivityBuilder.h"
#include "client/BusClient.h"
#include "client/Command.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailAccountAdapter.h"
#include "data/EmailSchema.h"
#include "data/EmailAdapter.h"
#include "data/FolderAdapter.h"
#include "exceptions/ExceptionUtils.h"
#include "data/SyncStateAdapter.h"
#include "client/SyncStateUpdater.h"

#include <boost/foreach.hpp>
#include <boost/exception_ptr.hpp>
#include "CommonPrivate.h"

using namespace std;

#define CATCH_AS_ERROR \
	catch(const exception& e) { HandleError(e, __FILE__, __LINE__); } \
	catch(...) { HandleError(boost::unknown_exception(), __FILE__, __LINE__); }

BaseSyncSession::BaseSyncSession(BusClient& busClient, MojLogger& logger, const MojObject& accountId, const MojObject& folderId)
: m_state(State_None),
  m_busClient(busClient),
  m_log(logger),
  m_accountId(accountId),
  m_folderId(folderId),
  m_foundFolder(false),
  m_stopRequested(false),
  m_startRequested(false),
  m_initialSync(false),
  m_lastSyncRev(0),
  m_nextSyncRev(0),
  m_activities(new ActivitySet(busClient)),
  m_queuedActivities(new ActivitySet(busClient)),
  m_readySignal(this),
  m_stoppedSignal(this),
  m_getFolderSlot(this, &BaseSyncSession::GetFolderResponse),
  m_startAccountSpinnerSlot(this, &BaseSyncSession::StartAccountSpinnerResponse),
  m_startFolderSpinnerSlot(this, &BaseSyncSession::StartFolderSpinnerResponse),
  m_adoptActivitiesSlot(this, &BaseSyncSession::ActivitiesAdopted),
  m_getNewChangesSlot(this, &BaseSyncSession::GetNewChangesResponse),
  m_endActivitiesSlot(this, &BaseSyncSession::ActivitiesEnded),
  m_updateFolderSlot(this, &BaseSyncSession::UpdateFolderResponse),
  m_stopAccountSpinnerSlot(this, &BaseSyncSession::StopAccountSpinnerResponse),
  m_stopFolderSpinnerSlot(this, &BaseSyncSession::StopFolderSpinnerResponse)
{
}

BaseSyncSession::~BaseSyncSession()
{
}

const char* BaseSyncSession::GetStateName(SyncSessionState state)
{
	switch(state) {
	case State_None: return "none";
	case State_Starting: return "starting";
	case State_Adopting: return "adopting";
	case State_Active: return "active";
	case State_Ending: return "ending";
	}

	return "unknown";
}

void BaseSyncSession::SetState(SyncSessionState state)
{
	MojLogDebug(m_log, "[sync session %s] setting state to \"%s\" (%d)", m_syncSessionName.c_str(), GetStateName(state), state);
	m_state = state;
}

void BaseSyncSession::RequestStart()
{
	if(m_state == State_None) {
		Start();
	}
}

void BaseSyncSession::RequestStop(MojSignal<>::SlotRef stoppedSlot)
{
	m_stoppedSignal.connect(stoppedSlot);

	RequestStop();
}

void BaseSyncSession::RequestStop()
{
	if(m_state == State_None) {
		// Fire signal immediately
		m_stoppedSignal.fire();
	} else if(m_state == State_Active) {
		// Force it to end. FIXME: need to wait for running commands to complete.
		End();
	} else {
		m_stopRequested = true;
	}
}

bool BaseSyncSession::IsFinished() const
{
	return m_state == State_None && m_waitingCommands.empty();
}

bool BaseSyncSession::IsActive() const
{
	return m_state == State_Active;
}

bool BaseSyncSession::IsRunning() const
{
	return m_state != State_None;
}

bool BaseSyncSession::IsStarting() const
{
	return m_state > State_None && m_state < State_Active;
}

bool BaseSyncSession::IsEnding() const
{
	return m_state > State_Active;
}

void BaseSyncSession::AdoptActivity(const ActivityPtr& activity)
{
	MojLogInfo(m_log, "[sync session %s] adopting activity %s", m_syncSessionName.c_str(), activity->Describe().c_str());

	if(m_state <= State_Adopting)
		m_activities->AddActivity(activity);
	else
		m_queuedActivities->AddActivity(activity);
}

bool BaseSyncSession::AddCommand(Command* command)
{
	// Check if command is already in list
	BOOST_FOREACH(const CommandPtr& commandPtr, m_waitingCommands) {
		if(commandPtr.get() == command) {
			// already waiting
			return false;
		}
	}

	BOOST_FOREACH(const CommandPtr& commandPtr, m_readyCommands) {
		if(commandPtr.get() == command) {
			// already running
			return true;
		}
	}

	if(m_state == State_Active) {
		// Add directly to running queue
		MojLogDebug(m_log, "[sync session %s] adding command %s to sync session active list", m_syncSessionName.c_str(), command->Describe().c_str());
		m_readyCommands.push_back(command);
		return true;
	} else {
		// Add to waiting commands which will get run when the next session starts
		MojLogDebug(m_log, "[sync session %s] adding command %s to sync session waiting list", m_syncSessionName.c_str(), command->Describe().c_str());
		m_waitingCommands.push_back(command);
		return false;
	}
}

void BaseSyncSession::RegisterCommand(Command* command)
{
	AddCommand(command);
}

void BaseSyncSession::RegisterCommand(Command* command, MojSignal<>::SlotRef readySlot)
{
	//MojLogInfo(m_log, "registering command %s with sync session", command->Describe().c_str());

	bool ready = AddCommand(command);

	if(ready) {
		// Fire immediately
		m_readySignal.connect(readySlot);
		m_readySignal.fire();
	} else {
		// Will get fired when the sync session is ready
		m_readySignal.connect(readySlot);
		m_startRequested = true;
	}

	if(m_state == State_None) {
		Start();
	}
}

void BaseSyncSession::CommandStarted(Command* command)
{
	// currently doesn't do anything
}

void BaseSyncSession::CommandFailed(Command* command, const exception& e)
{
	m_failedCommands[command] = ExceptionUtils::CopyException(e);

	MojLogInfo(m_log, "[sync session %s] command %s failed: %s", m_syncSessionName.c_str(), command->Describe().c_str(), e.what());
}

void BaseSyncSession::CommandCompleted(Command* command)
{
	MojLogDebug(m_log, "[sync session %s] command %s completed", m_syncSessionName.c_str(), command->Describe().c_str());

	vector<CommandPtr>::iterator it;
	for(it = m_readyCommands.begin(); it != m_readyCommands.end(); ++it) {
		if(it->get() == command) {
			m_readyCommands.erase(it);
			break;
		}
	}

	if(m_readyCommands.empty() && m_state == State_Active) {
		// All done
		End();
	}
}

void BaseSyncSession::Start()
{
	m_syncSessionName = AsJsonString(m_folderId);

	if(m_state == State_None) {
		SetState(State_Starting);

		GetFolderData();
	} else {
		throw MailException("attempted to start sync session in bad state", __FILE__, __LINE__);
	}
}

void BaseSyncSession::GetFolderData()
{
	m_foundFolder = false;

	GetById(m_getFolderSlot, m_folderId);
}

MojErr BaseSyncSession::GetFolderResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojObject folder;
		DatabaseAdapter::GetOneResult(response, folder);

		m_foundFolder = true;

		m_lastSyncRev = 0;
		if(folder.contains("lastSyncRev")) {
			err = folder.getRequired("lastSyncRev", m_lastSyncRev);
			ErrorToException(err);
		} else {
			// Initial sync if lastSyncRev property doesn't exist
			// We should make sure a lastSyncrev is set after sync even if the folder has no emails,
			// but not if the sync failed.
			m_initialSync = true;
		}

		StartSpinner();
	} CATCH_AS_ERROR

	return MojErrNone;
}

void BaseSyncSession::StartSpinner()
{
	if(m_initialSync && IsAccountSync())
		StartAccountSpinner();
	else
		StartFolderSpinner();
}

void BaseSyncSession::StartAccountSpinner()
{
	m_syncStateUpdater.reset(new SyncStateUpdater(m_busClient, GetCapabilityProvider().data(), GetBusAddress().data()));
	m_syncStateUpdater->UpdateSyncState(m_startAccountSpinnerSlot, GetAccountSyncState());
}

MojErr BaseSyncSession::StartAccountSpinnerResponse()
{
	try {
		StartFolderSpinner();
	} CATCH_AS_ERROR

	return MojErrNone;
}

void BaseSyncSession::StartFolderSpinner()
{
	m_syncStateUpdater.reset(new SyncStateUpdater(m_busClient, GetCapabilityProvider().data(), GetBusAddress().data()));
	m_syncStateUpdater->UpdateSyncState(m_startFolderSpinnerSlot, GetFolderSyncState());
}

MojErr BaseSyncSession::StartFolderSpinnerResponse()
{
	try {
		StartSpinnerDone();
	} CATCH_AS_ERROR

	return MojErrNone;
}

void BaseSyncSession::StartSpinnerDone()
{
	SetState(State_Adopting);
	AdoptActivities();
}

SyncState BaseSyncSession::GetFolderSyncState()
{
	SyncState syncState(m_accountId, m_folderId);

	if(this->IsStarting())
		syncState.SetState(m_initialSync ? SyncState::INITIAL_SYNC : SyncState::INCREMENTAL_SYNC);
	else // ending
		syncState.SetState(SyncState::IDLE);

	return syncState;
}

SyncState BaseSyncSession::GetAccountSyncState()
{
	SyncState syncState(m_accountId);

	if(this->IsStarting())
		syncState.SetState(m_initialSync ? SyncState::INITIAL_SYNC : SyncState::INCREMENTAL_SYNC);
	else // ending
		syncState.SetState(SyncState::IDLE);

	return syncState;
}

// Indicates whether this counts as an account sync
// FIXME there ought to be a cleaner way to do this
bool BaseSyncSession::IsAccountSync()
{
	return true;
}

void BaseSyncSession::AdoptActivities()
{
	// Move queued activities to activity list
	m_queuedActivities->PassActivities(*m_activities);

	// Adopt activities
	m_activities->StartActivities(m_adoptActivitiesSlot);
}

MojErr BaseSyncSession::ActivitiesAdopted()
{
	// TODO check for error adopting

	SetState(State_Active);

	if(m_stopRequested)
		End();
	else
		SyncSessionReady();

	return MojErrNone;
}

void BaseSyncSession::SyncSessionReady()
{
	MojLogDebug(m_log, "[sync session %s] ready", m_syncSessionName.c_str());

	// All waiting commands are now running commands
	m_readyCommands.clear();
	swap(m_waitingCommands, m_readyCommands);

	m_readySignal.fire();

	// FIXME: enable this once IMAP and POP manage adding commands properly
#if 0
	if(m_readyCommands.empty()) {
		MojLogInfo(m_log, "[sync session %s] no commands in session; ending", m_syncSessionName.c_str());
		End();
	}
#endif
}

MojInt64 BaseSyncSession::GetLastSyncRev() const
{
	if(m_state == State_Active) {
		return m_lastSyncRev;
	} else {
		throw MailException("called GetLastSyncRev in wrong state", __FILE__, __LINE__);
	}
}

void BaseSyncSession::AddPutResponseRevs(const MojObject& response)
{
	BOOST_FOREACH(const MojObject& entry, DatabaseAdapter::GetResultsIterators(response)) {
		MojInt64 rev;
		MojErr err = entry.getRequired(DatabaseAdapter::RESULT_REV, rev);
		ErrorToException(err);

		m_knownNewRevs.insert(rev);
	}
}

void BaseSyncSession::SetNextSyncRev(MojInt64 rev)
{
	m_nextSyncRev = rev;
}

void BaseSyncSession::End()
{
	if(m_state == State_Active) {
		SetState(State_Ending);

		GetNewChanges();
	} else {
		throw MailException("called End in wrong state", __FILE__, __LINE__);
	}
}

void BaseSyncSession::GetNewChanges()
{
	MojInt64 rev = std::max(m_lastSyncRev, m_nextSyncRev);
	GetNewChanges(m_getNewChangesSlot, m_folderId, rev, m_changesPage);
}

MojErr BaseSyncSession::GetNewChangesResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		int newChanges = 0;
		int knownChanges = 0;

		MojInt64 lowestNewRev = MojInt64Max;
		MojInt64 highestRev = 0;

		// Go through the changes, and find any changes that weren't generated by our commands.
		BOOST_FOREACH(const MojObject& obj, DatabaseAdapter::GetResultsIterators(response)) {
			MojInt64 rev;
			err = obj.getRequired(DatabaseAdapter::REV, rev);
			ErrorToException(err);

			if(m_knownNewRevs.find(rev) != m_knownNewRevs.end()) {
				// Ignore this update; we already know about it
				knownChanges++;

				if(rev > highestRev) {
					highestRev = rev;
				}
			} else {
				// New update
				newChanges++;

				if(rev < lowestNewRev) {
					lowestNewRev = rev;
				}
			}
		}

		if(newChanges > 0) {
			// Need to re-examine everything from the first new rev
			m_lastSyncRev = lowestNewRev - 1;
		} else {
			// all revisions accounted for, or the list is empty
			m_lastSyncRev = std::max(m_lastSyncRev, highestRev);
		}

		// Should be at least a high as the next sync rev (in case the list is empty)
		m_lastSyncRev = std::max(m_lastSyncRev, m_nextSyncRev);

		MojLogInfo(m_log, "[sync session %s] ending; %d new local changes not handled yet, %d existing changes", m_syncSessionName.c_str(), newChanges, knownChanges);

		// FIXME handle changes that haven't been accounted for
		UpdateFolder();
	} CATCH_AS_ERROR

	return MojErrNone;
}

void BaseSyncSession::BuildFolderStatus(MojObject& folderStatus)
{
	MojErr err;

	// Subclasses should provide actual state (pushing, scheduled, or manual)
	err = folderStatus.putString(FolderAdapter::SYNC_STATUS, FolderAdapter::SYNC_STATUS_MANUAL);
	ErrorToException(err);
}

void BaseSyncSession::UpdateFolder()
{
	MojErr err;

	// Update folder sync status and last sync rev
	MojObject folderStatus;
	err = folderStatus.put(DatabaseAdapter::ID, m_folderId);
	ErrorToException(err);

	// Set sync status info
	BuildFolderStatus(folderStatus);

	// Update last sync rev with the highest revision we've updated
	if(m_lastSyncRev > 0) {
		err = folderStatus.put("lastSyncRev", m_lastSyncRev);
		ErrorToException(err);
	} else if(!m_stopRequested) {
		// Need to set last sync rev even if there's nothing in the folder,
		// except if the sync was aborted.
		err = folderStatus.put("lastSyncRev", 0);
		ErrorToException(err);
	}

	if(true) {
		// FIXME make sure sync was successful

		// FIXME wrap this in a function
		struct timespec ts;
		clock_gettime(CLOCK_REALTIME, &ts);

		err = folderStatus.put(FolderAdapter::LAST_SYNC_TIME, (MojInt64) ts.tv_sec * 1000L);
		ErrorToException(err);
	}

	Merge(m_updateFolderSlot, folderStatus);
}

MojErr BaseSyncSession::UpdateFolderResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		UpdateAndEndActivities();
	} CATCH_AS_ERROR

	return MojErrNone;
}

void BaseSyncSession::UpdateAndEndActivities()
{
	UpdateActivities();
	EndActivities();
}

void BaseSyncSession::UpdateActivities()
{
	// This should be overridden. By default, activities will be completed without restarting them.
}

void BaseSyncSession::EndActivities()
{
	m_activities->EndActivities(m_endActivitiesSlot);
}

MojErr BaseSyncSession::ActivitiesEnded()
{
	StopSpinner();

	return MojErrNone;
}

void BaseSyncSession::StopSpinner()
{
	if(m_initialSync && IsAccountSync())
		StopAccountSpinner();
	else
		StopFolderSpinner();
}

void BaseSyncSession::StopAccountSpinner()
{
	m_syncStateUpdater.reset(new SyncStateUpdater(m_busClient, GetCapabilityProvider().data(), GetBusAddress().data()));
	m_syncStateUpdater->UpdateSyncState(m_stopAccountSpinnerSlot, GetAccountSyncState());
}

MojErr BaseSyncSession::StopAccountSpinnerResponse()
{
	try {
		StopFolderSpinner();
	} CATCH_AS_ERROR

	return MojErrNone;
}

void BaseSyncSession::StopFolderSpinner()
{
	m_syncStateUpdater.reset(new SyncStateUpdater(m_busClient, GetCapabilityProvider().data(), GetBusAddress().data()));
	m_syncStateUpdater->UpdateSyncState(m_stopFolderSpinnerSlot, GetFolderSyncState());
}

MojErr BaseSyncSession::StopFolderSpinnerResponse()
{
	try {
		StopSpinnerDone();
	} CATCH_AS_ERROR

	return MojErrNone;
}

void BaseSyncSession::StopSpinnerDone()
{
	SetState(State_None);
	SyncSessionComplete();
}

void BaseSyncSession::Cleanup()
{
	m_activities->ClearActivities();

	// Clear revision set
	m_knownNewRevs.clear();

	m_readyCommands.clear();
	m_failedCommands.clear();
}

void BaseSyncSession::SyncSessionComplete()
{
	MojLogInfo(m_log, "[sync session %s] complete", m_syncSessionName.c_str());

	Cleanup();

	m_stoppedSignal.fire();

	if(!m_waitingCommands.empty() && !m_stopRequested && m_startRequested) {
		MojLogNotice(m_log, "[sync session %s] has commands queued; starting up again", m_syncSessionName.c_str());

		m_stopRequested = false;
		m_startRequested = false;
		Start();
	}

	m_startRequested = false;
	m_stopRequested = false;
	m_initialSync = false;
}

void BaseSyncSession::HandleError(const exception& e, const char* file, int line)
{
	MojLogError(m_log, "exception in sync session caught at %s:%d: %s", file, line, e.what());
	MojLogError(m_log, "attempting to clean up; current state: %s", GetStateName(m_state));

	m_lastError = ExceptionUtils::GetErrorInfo(e);

	try {

		switch(m_state) {
		case State_None:
		case State_Active:
			// do nothing
			break;
		case State_Starting:
		case State_Adopting:
			if(m_foundFolder) {
				SetState(State_Active);
				End();
			} else {
				// No folder -- end activities
				SetState(State_Ending);
				m_activities->SetEndAction(Activity::EndAction_Cancel);
				EndActivities();
			}

			// This should cause any waiting commands to fail
			m_readySignal.fire();

			break;
		case State_Ending:
			SetState(State_None);
			SyncSessionComplete();
			break;
		}

	} catch(const exception& ee) {
		MojLogCritical(m_log, "error trying to clean up sync session: %s", ee.what());
	}
}

void BaseSyncSession::HandleError(const char* file, int line)
{
	// this should never happen
	HandleError(boost::unknown_exception(), file, line);
}

void BaseSyncSession::Status(MojObject& status) const
{
	MojErr err;

	err = status.put("folderId", m_folderId);
	ErrorToException(err);

	err = status.putString("syncSessionState", GetStateName(m_state));
	ErrorToException(err);

	MojObject activitySetStatus;
	m_activities->Status(activitySetStatus);
	err = status.put("activitySet", activitySetStatus);
	ErrorToException(err);

	if(!m_waitingCommands.empty()) {
		MojObject waitingCommands(MojObject::TypeArray);
		BOOST_FOREACH(const CommandPtr& command, m_waitingCommands) {
			MojObject commandStatus;
			command->Status(commandStatus);
			err = waitingCommands.push(commandStatus);
			ErrorToException(err);
		}

		err = status.put("waitingCommands", waitingCommands);
		ErrorToException(err);
	}

	if(!m_readyCommands.empty()) {
		MojObject readyCommands(MojObject::TypeArray);
		BOOST_FOREACH(const CommandPtr& command, m_readyCommands) {
			MojObject commandStatus;
			command->Status(commandStatus);
			err = readyCommands.push(commandStatus);
			ErrorToException(err);
		}

		err = status.put("readyCommands", readyCommands);
		ErrorToException(err);
	}

	if(m_lastError.errorCode) {
		MojObject errorObj;

		EmailAccountAdapter::SerializeErrorInfo(m_lastError, errorObj);

		err = status.put("lastError", errorObj);
		ErrorToException(err);
	}
}
