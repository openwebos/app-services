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

#ifndef ACTIVITY_H_
#define ACTIVITY_H_

#include "core/MojCoreDefs.h"
#include "core/MojObject.h"
#include "core/MojRefCount.h"
#include "core/MojServiceRequest.h"
#include <string>

class ActivityBuilder;
class BusClient;

class ActivityHandler
{
public:
	virtual void ActivityStarted() = 0;
};

class Activity : public MojSignalHandler
{
public:
	Activity();
	virtual ~Activity();
	
	enum ActivityPurpose {
		GenericActivity,
		WatchActivity,
		ScheduledSyncActivity
	};

	enum StateType {
		s_NeedsCreation,
		s_NeedsAdoption,
		s_AdoptPending,
		s_CreatePending,
		s_Waiting,			// waiting for start
		s_Active,
		s_CompletePending,
		s_CancelPending,
		s_Ended
	};
	
	enum EventType {
		StartEvent,
		UpdateEvent,
		OrphanEvent,
		CompleteEvent,
		StopEvent,
		CancelEvent,
		YieldEvent,
		PauseEvent
	};
	
	enum ErrorType {
		UnknownError,
		AdoptError,		// error trying to adopt an activity
		CreateError,	// error trying to create an activity
		CompleteError	// error trying to complete an activity
	};
	
	enum EndAction {
		EndAction_Complete,		// complete without restarting
		EndAction_Restart,		// complete with restarting (needs updated payload)
		EndAction_Cancel,		// cancel the activity (TODO)
		EndAction_Unsubscribe	// just unsubscribe
	};

	enum EndOrder {
		EndOrder_First,		// end before other activities
		EndOrder_Default,	// no order preference
		EndOrder_Last		// end after other activities have ended
	};

	static const char* GetEventTypeName(EventType type);
	static const char* GetErrorTypeName(ErrorType type);
	
	typedef MojSignal<Activity*, EventType> UpdateSignal;
	typedef MojSignal<Activity*, ErrorType, const std::exception&> ErrorSignal;
	
	// Start an activity
	void Create(BusClient& client);
	void Adopt(BusClient& client);
	void Complete(BusClient& client);
	void UpdateAndComplete(BusClient& client, const MojObject& updatedFields);

	// Replaces an activity, completing it if necessary. The activity will be considered
	// complete after the activity is replaced.
	void Replace(BusClient& client, const MojObject& updatedFields);

	// Cancels an activity
	void Cancel(BusClient& client);

	// Adopts or creates an activity, assuming the activity was created with
	// either PrepareNewActivity or PrepareAdoptedActivity.
	void Start(BusClient& client);

	// End the activity using the action defined using SetEndAction.
	// Defaults to EndAction_Unsubscribe
	void End(BusClient& client);

	// Unsubscribes the activity
	void Unsubscribe();

	// Set the action that should be performed when Activity::End() is called.
	void SetEndAction(EndAction endAction);

	// Set the order that the activity should be ended relative to other activities.
	void SetEndOrder(EndOrder endOrder) { m_endOrder = endOrder; }

	EndOrder GetEndOrder() const { return m_endOrder; }

	// Set the activity details (callback, requirements, etc) to be used on Activity::End
	void SetUpdatePayload(const MojObject& updatedFields);

	// Creates an Activity which can be used to create a new activity via the Create method
	static MojRefCountedPtr<Activity> PrepareNewActivity(ActivityBuilder& builder, bool start = true, bool replace = true);

	// Creates an Activity which will adopt an activityId instead of creating a new activity.
	static MojRefCountedPtr<Activity> PrepareAdoptedActivity(const MojObject& activityId);
	
	// Creates an Activity which will adopt an activityId instead of creating a new activity.
	static MojRefCountedPtr<Activity> PrepareAdoptedActivity(const MojString& name);

	// Connect the slot to listen for activity updates
	// Note that any previously connected slots *must* be cancelled first
	void SetSlots(UpdateSignal::SlotRef updateSlot, ErrorSignal::SlotRef errorSlot);
	
	bool IsActive() const;
	
	bool IsStarting() const;
	bool IsWaitingToStart() const; // whether we're waiting to start
	bool IsEnding() const;
	bool IsEnded() const;

	bool CanAdopt() const;
	bool CanCreate() const;
	bool CanStart() const;

	std::string Describe() const;

	bool IsParent() const { return m_isParent; }

	static const char* GetStateName(StateType state);
	void Status(MojObject& status) const;

	void SetPurpose(ActivityPurpose purpose);
	ActivityPurpose GetPurpose() const { return m_purpose; }

	void SetName(const MojString& name) { m_activityName = name; }
	const MojString& GetName() const { return m_activityName; }
	
	void SetInfo(const MojObject& info) { m_activityInfo = info; }
	const MojObject& GetInfo() const { return m_activityInfo; }

	static MojLogger	s_log;

	static const char* GetEndActionName(EndAction endAction);
	static const char* GetEndOrderName(EndOrder endOrder);

	const MojObject& GetActivityId() const { return m_activityId; }

protected:
	void PutIdOrName(MojObject& payload);
	void Adopt(BusClient& client, const MojObject& activityId);
	
	MojErr CreateOrAdoptResponse(MojObject& response, MojErr err);
	MojErr CompleteResponse(MojObject& response, MojErr err);
	MojErr ReplaceResponse(MojObject& response, MojErr err);
	MojErr CancelResponse(MojObject& response, MojErr err);
	
	void HandleUpdate(MojObject& response);
	ErrorType GetErrorForState(StateType state);
	void ReportError(ErrorType, const std::exception&);

	StateType			m_state;

	ActivityPurpose		m_purpose;
	EndAction			m_endAction;
	EndOrder			m_endOrder;
	MojString			m_activityName;

	// Used for creating activities
	MojObject			m_newActivity;

	// Payload used to update the activity when it ends (replace, restart)
	MojObject			m_updatePayload;

	MojObject			m_activityId;
	MojObject			m_serial;
	MojObject			m_creator;

	// Whether we're the parent
	bool				m_isParent;
	
	MojObject			m_activityInfo;		// copy of $activity state payload

	// Slots
	MojServiceRequest::ReplySignal::Slot<Activity>		m_subscriptionSlot;
	MojServiceRequest::ReplySignal::Slot<Activity>		m_completedSlot;
	MojServiceRequest::ReplySignal::Slot<Activity>		m_replaceSlot;
	MojServiceRequest::ReplySignal::Slot<Activity>		m_cancelSlot;
	
	UpdateSignal		m_updateSignal;
	ErrorSignal			m_errorSignal;
};

typedef MojRefCountedPtr<Activity> ActivityPtr;

#endif /*ACTIVITYCLIENT_H_*/
