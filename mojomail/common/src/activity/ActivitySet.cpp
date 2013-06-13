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

#include "activity/ActivitySet.h"
#include "activity/Activity.h"
#include "exceptions/ExceptionUtils.h"
#include "CommonPrivate.h"
#include <boost/foreach.hpp>

using namespace std;

MojLogger& ActivitySet::s_log = Activity::s_log;

ActivitySet::ActivitySet(BusClient& busClient)
: m_busClient(busClient),
  m_state(State_None),
  m_startedSignal(this),
  m_endedSignal(this),
  m_updateSignal(this)
{
}

ActivitySet::~ActivitySet()
{
}

void ActivitySet::AddActivity(const ActivityPtr& activity)
{
	if(activity.get() == NULL) {
		throw MailException("AddActivity called with null activity", __FILE__, __LINE__);
	}

	// Only add if it's not already in the set
	if(FindActivitySlot(activity.get()) == m_activities.end()) {
		ActivitySlotPtr activityState(new ActivitySlot(this, activity));
		m_activities.push_back(activityState);

		if(activity->IsStarting()) {
			m_starting.insert(activity);
		} else if(activity->IsEnding()) {
			m_ending.insert(activity);
		}
	}
}

void ActivitySet::AddActivities(const std::vector<ActivityPtr>& activities)
{
	BOOST_FOREACH(const ActivityPtr& activity, activities) {
		AddActivity(activity);
	}
}

void ActivitySet::RemoveActivity(const ActivityPtr& activity)
{
	vector<ActivitySlotPtr>::iterator it = FindActivitySlot(activity.get());

	if(it != m_activities.end()) {
		m_activities.erase(it);
	}

	m_starting.erase(activity);
	m_ending.erase(activity);

	m_endFirst.erase(activity);
	m_endDefault.erase(activity);
	m_endLast.erase(activity);

	// Check if there's no remaining activities
	if(m_state == State_Starting) {
		CheckStartDone();
	} else if(m_state == State_Ending) {
		CheckEndDone();
	}
}

void ActivitySet::ClearActivities()
{
	m_activities.clear();
}

void ActivitySet::StartActivities(MojSignal<>::SlotRef doneSlot)
{
	if(m_state != State_None) {
		throw MailException("activity set already starting or ending", __FILE__, __LINE__);
	}

	m_state = State_Starting;
	m_startedSignal.connect(doneSlot);

	BOOST_FOREACH(const ActivitySlotPtr& activityState, m_activities) {
		ActivityPtr& activity = activityState->m_activity;

		if(activity->IsStarting()) {
			// already starting
			MojLogDebug(s_log, "%s: %s already starting", __PRETTY_FUNCTION__, activity->Describe().c_str());

			m_starting.insert(activity.get());
		} else if(activity->CanStart()) {
			MojLogDebug(s_log, "%s: starting %s", __PRETTY_FUNCTION__, activity->Describe().c_str());

			m_starting.insert(activity.get());

			// start (create or adopt) the activity
			activity->Start(m_busClient);
		}
	}

	if(m_starting.empty()) {
		// We don't expect any callbacks, so check if we're done right now
		CheckStartDone();
	}
}

void ActivitySet::CheckStartDone()
{
	if(m_starting.empty()) {
		// All activites have been started, or at least don't need to start
		m_state = State_None;
		m_startedSignal.fire();
	}
}

void ActivitySet::QueueEndActivity(const ActivityPtr& activity)
{
	switch(activity->GetEndOrder()) {
	case Activity::EndOrder_First:
		m_endFirst.insert(activity);
		break;
	case Activity::EndOrder_Default:
		m_endDefault.insert(activity);
		break;
	case Activity::EndOrder_Last:
		m_endLast.insert(activity);
		break;
	}
}

void ActivitySet::EndActivities(MojSignal<>::SlotRef doneSlot)
{
	if(m_state != State_None) {
		throw MailException("activity set already starting or ending", __FILE__, __LINE__);
	}

	m_state = State_Ending;
	m_endedSignal.connect(doneSlot);

	BOOST_FOREACH(const ActivitySlotPtr& activityState, m_activities) {
		ActivityPtr& activity = activityState->m_activity;

		if(activity->IsEnding()) {
			m_ending.insert(activity);
		} else if(activity->IsStarting() && !activity->IsWaitingToStart()) {
			// If the activity is starting up, we need to wait for it to finish first.
			// Exception: if the activity was created but hasn't started, we should be able to end it
			// since it might be waiting on network or some other condition to start.
		} else {
			if(activity->IsWaitingToStart()) {
				// If the activity is waiting on some requirement to start, allow ending it immediately.
				// First we need to remove it from the starting queue so we don't wait for it to start.
				m_starting.erase(activity);
			}

			QueueEndActivity(activity);
		}
	}

	CheckEndDone();
}

void ActivitySet::EndSomeActivities(set<ActivityPtr>& activities)
{
	// Note, this could be called recursively
	while(!activities.empty()) {
		set<ActivityPtr>::iterator it = activities.begin();
		ActivityPtr activity = *it;
		activities.erase(it);

		m_ending.insert(activity);

		if(!activity->IsEnding()) {
			activity->End(m_busClient);
		}
	}
}

void ActivitySet::CheckEndDone()
{
	if(!m_starting.empty()) {
		// Wait for activities to finish starting before ending any
		MojLogDebug(s_log, "%s: waiting for %d activities to finish starting", __PRETTY_FUNCTION__, m_starting.size());
	} else if(!m_ending.empty()) {
		// Wait for currently ending activities to finish ending before ending more
		MojLogDebug(s_log, "%s: waiting for %d activities to end", __PRETTY_FUNCTION__, m_ending.size());
	} else if(!m_endFirst.empty()) {
		MojLogDebug(s_log, "%s: ending EndFirst activities", __PRETTY_FUNCTION__);
		EndSomeActivities(m_endFirst);
	} else if(!m_endDefault.empty()) {
		MojLogDebug(s_log, "%s: ending EndDefault activities", __PRETTY_FUNCTION__);
		EndSomeActivities(m_endDefault);
	} else if(!m_endLast.empty()) {
		MojLogDebug(s_log, "%s: ending EndLast activities", __PRETTY_FUNCTION__);
		EndSomeActivities(m_endLast);
	} else {
		MojLogDebug(s_log, "%s: all activities ended", __PRETTY_FUNCTION__);

		// All activities ended
		m_state = State_None;
		m_endedSignal.fire();
	}
}

ActivityPtr ActivitySet::FindActivityByName(const MojString& name)
{
	BOOST_FOREACH(const ActivitySlotPtr& activityState, m_activities) {
		const ActivityPtr& activity = activityState->m_activity;
		if(activity->GetName() == name) {
			return activity;
		}
	}

	return NULL;
}

ActivityPtr ActivitySet::GetOrCreateActivity(const MojString& name)
{
	ActivityPtr activity = FindActivityByName(name);

	if(activity.get() == NULL) {
		activity.reset(new Activity());
		activity->SetName(name);

		AddActivity(activity);
	}

	return activity;
}

void ActivitySet::ReplaceActivity(const MojString& name, const MojObject& updatedFields)
{
	ActivityPtr activity = GetOrCreateActivity(name);

	activity->SetEndAction(Activity::EndAction_Restart);
	activity->SetUpdatePayload(updatedFields);
}

void ActivitySet::CancelActivity(const MojString& name)
{
	ActivityPtr activity = GetOrCreateActivity(name);

	activity->SetEndAction(Activity::EndAction_Cancel);
}

vector<ActivityPtr> ActivitySet::GetActivities() const
{
	vector<ActivityPtr> vec;

	BOOST_FOREACH(const ActivitySlotPtr& activityState, m_activities) {
		vec.push_back(activityState->m_activity);
	}

	return vec;
}

void ActivitySet::PassActivities(ActivitySet& other)
{
	BOOST_FOREACH(const ActivitySlotPtr& activityState, m_activities) {
		other.AddActivity(activityState->m_activity);
	}
	m_activities.clear();
}

vector<ActivitySet::ActivitySlotPtr>::iterator ActivitySet::FindActivitySlot(Activity* activity)
{
	vector<ActivitySlotPtr>::iterator it;

	for(it = m_activities.begin(); it != m_activities.end(); ++it) {
		if((*it)->m_activity.get() == activity)
			return it;
	}

	return m_activities.end();
}

MojErr ActivitySet::ActivityUpdate(Activity* activity, Activity::EventType event)
{
	if(event == Activity::StartEvent) {
		MojLogDebug(s_log, "%s: %s started", __PRETTY_FUNCTION__, activity->Describe().c_str());

		m_starting.erase(activity);

		CheckStartDone();

		if(m_state == State_Ending) {
			// This happens if EndActivities was called while activities were still starting.
			// Now that it's started, the activity in the appropriate end queue.
			QueueEndActivity(activity);
			CheckEndDone();
		}
	} else if(event == Activity::CompleteEvent) {
		MojLogDebug(s_log, "%s: %s ended", __PRETTY_FUNCTION__, activity->Describe().c_str());

		// Remove activity -- this will call CheckEndDone()
		RemoveActivity(activity);
	} else if(event == Activity::CancelEvent) {
		MojLogDebug(s_log, "%s: %s cancelled", __PRETTY_FUNCTION__, activity->Describe().c_str());

		// Remove activity
		RemoveActivity(activity);
	}

	m_updateSignal.call(activity, event);

	return MojErrNone;
}

MojErr ActivitySet::ActivityError(Activity* activity, Activity::ErrorType event, const std::exception& e)
{
	MojLogError(s_log, "activity error in activity set: %s: %s", activity->Describe().c_str(), e.what());
	m_exception = ExceptionUtils::CopyException(e);

	// For now, just remove the activity
	RemoveActivity(activity);

	return MojErrNone;
}

ActivitySet::ActivitySlot::ActivitySlot(ActivitySet* activitySet, const ActivityPtr& activity)
: m_activity(activity),
  m_updateSlot(activitySet, &ActivitySet::ActivityUpdate),
  m_errorSlot(activitySet, &ActivitySet::ActivityError)
{
	activity->SetSlots(m_updateSlot, m_errorSlot);
}

ActivitySet::ActivitySlot::~ActivitySlot()
{
	m_updateSlot.cancel();
	m_errorSlot.cancel();
}

void ActivitySet::ConnectUpdateSlot(Activity::UpdateSignal::SlotRef updateSlot)
{
	m_updateSignal.connect(updateSlot);
}

void ActivitySet::Status(MojObject& status) const
{
	MojErr err;
	MojObject activities(MojObject::TypeArray);

	BOOST_FOREACH(const ActivitySlotPtr& activityState, m_activities) {
		MojObject activityStatus;
		activityState->m_activity->Status(activityStatus);

		err = activities.push(activityStatus);
		ErrorToException(err);
	}

	err = status.put("activities", activities);
	ErrorToException(err);

	const char* state = "unknown";
	switch(m_state) {
	case State_None: state = "none"; break;
	case State_Starting: state = "starting"; break;
	case State_Ending: state = "ending"; break;
	}

	err = status.putString("activitySetState", state);
	ErrorToException(err);

	if(m_state == State_Starting) {
		MojObject starting(MojObject::TypeArray);

		BOOST_FOREACH(const ActivityPtr& activity, m_starting) {
			MojObject activityStatus;
			activity->Status(activityStatus);

			err = starting.push(activityStatus);
			ErrorToException(err);
		}

		err = status.put("starting", starting);
		ErrorToException(err);
	} else if(m_state == State_Ending) {
		MojObject ending(MojObject::TypeArray);

		BOOST_FOREACH(const ActivityPtr& activity, m_ending) {
			MojObject activityStatus;
			activity->Status(activityStatus);

			err = ending.push(activityStatus);
			ErrorToException(err);
		}

		err = status.put("ending", ending);
		ErrorToException(err);
	}
}

void ActivitySet::SetEndAction(Activity::EndAction endAction)
{
	BOOST_FOREACH(const ActivitySlotPtr& state, m_activities) {
		state->m_activity->SetEndAction(endAction);
	}
}
