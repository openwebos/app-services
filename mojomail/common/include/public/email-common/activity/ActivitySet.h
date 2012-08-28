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

#ifndef ACTIVITYSET_H_
#define ACTIVITYSET_H_

#include "activity/Activity.h"
#include <vector>
#include <set>
#include <boost/exception_ptr.hpp>

/**
 * ActivitySet allows starting and ending a group of activities.
 *
 * The basic functions are:
 *
 * AddActivity		- Adds an activity to the set. Does not start the activity.
 * RemoveActivity 	- Remove an activity from the set. Does not affect the activity state.
 *
 * StartActivities	- Start all activities that haven't been started already.
 * EndActivities	- End all started activities.
 */
class ActivitySet : public MojSignalHandler
{
	// Internal state, to prevent adopting and ending at the same time
	// Note that individual activities may be in any state.
	enum State
	{
		State_None,
		State_Starting,
		State_Ending
	};

	class ActivitySlot : public MojSignalHandler
	{
	public:
		ActivitySlot(ActivitySet* activitySet, const ActivityPtr& activity);
		virtual ~ActivitySlot();

		ActivityPtr			m_activity;

		Activity::UpdateSignal::Slot<ActivitySet>		m_updateSlot;
		Activity::ErrorSignal::Slot<ActivitySet>		m_errorSlot;
	};

	typedef MojRefCountedPtr<ActivitySlot> ActivitySlotPtr;

public:
	ActivitySet(BusClient& busClient);
	virtual ~ActivitySet();

	// Add an activity
	void AddActivity(const ActivityPtr& activity);

	// Add activities
	void AddActivities(const std::vector<ActivityPtr>& activity);

	// Remove an activity
	void RemoveActivity(const ActivityPtr& activity);

	// Clear all activities
	void ClearActivities();

	// Transfer activities to another activity set
	void PassActivities(ActivitySet& other);

	// Create or update an activity with the given name.
	// The activity will be updated when EndActivities is called.
	void ReplaceActivity(const MojString& name, const MojObject& payload);

	// Cancel activity with the given name
	void CancelActivity(const MojString& name);

	// Adopts activities that are able to be adopted
	void StartActivities(MojSignal<>::SlotRef doneSlot);

	// Completes activities. If an activity is in the process of starting, it will be ended
	// after the start completes.
	void EndActivities(MojSignal<>::SlotRef doneSlot);

	// Find an activity by name
	ActivityPtr FindActivityByName(const MojString& name);

	// Find an activity by name, or create one if it doesn't exist
	ActivityPtr GetOrCreateActivity(const MojString& name);

	// Get all activities in the set
	std::vector<ActivityPtr> GetActivities() const;

	// Connect slots to get update events
	void ConnectUpdateSlot(Activity::UpdateSignal::SlotRef updateSlot);

	void Status(MojObject& status) const;

	// Sets the end action on all activities currently in the set
	void SetEndAction(Activity::EndAction endAction);

protected:
	std::vector<ActivitySlotPtr>::iterator FindActivitySlot(Activity* activity);

	bool ActivitiesStarting();
	void CheckStartDone();

	void QueueEndActivity(const ActivityPtr& activity);
	void EndSomeActivities(std::set<ActivityPtr>& list);
	void CheckEndDone();

	MojErr ActivityUpdate(Activity* activity, Activity::EventType event);
	MojErr ActivityError(Activity* activity, Activity::ErrorType error, const std::exception& e);

	BusClient&	m_busClient;
	State		m_state;

	std::vector<ActivitySlotPtr>	m_activities;

	// Activities that are starting
	std::set<ActivityPtr>	m_starting;

	// Activities that we're trying to end
	std::set<ActivityPtr>	m_ending;

	// Activities queued for ending
	std::set<ActivityPtr>	m_endFirst;
	std::set<ActivityPtr>	m_endDefault;
	std::set<ActivityPtr>	m_endLast;

	boost::exception_ptr	m_exception;

	static MojLogger&		s_log;

	MojSignal<>		m_startedSignal;
	MojSignal<>		m_endedSignal;

	Activity::UpdateSignal	m_updateSignal;
};

#endif /* ACTIVITYSET_H_ */
