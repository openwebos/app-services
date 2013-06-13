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

#include "CommonPrivate.h"
#include "exceptions/MailException.h"
#include "activity/ActivityBuilder.h"
#include "client/PowerManager.h"
#include "client/BusClient.h"

using namespace std;

MojLogger PowerManager::s_log("com.palm.mail.powermanager");
int PowerManager::s_powerActivityCount = 1;

PowerManager::PowerManager(BusClient& client)
: m_client(client),
  m_stayAwakeCount(0),
  m_enabled(true),
  m_state(State_NoActivity),
  m_activityUpdateSlot(this, &PowerManager::ActivityUpdate),
  m_activityErrorSlot(this, &PowerManager::ActivityException)
{
}

PowerManager::~PowerManager()
{

}

void PowerManager::StayAwake(bool enabled, string reason)
{
	if (!m_enabled) {
		return;
	}

	if (enabled) {
		m_stayAwakeCount++;
		MojLogInfo(s_log, "keeping the device awake: %s (users: %i)",
				reason.c_str(), m_stayAwakeCount);

		if (m_stayAwakeCount == 1)
			CreateActivity();
	} else {
		m_stayAwakeCount--;

		if (m_stayAwakeCount < 0) {
			MojLogCritical(s_log, "unmatched power request for %s", reason.c_str());
			throw new MailException("unmatched power request", __FILE__,
					__LINE__);
		} else if (m_stayAwakeCount == 0) {
			MojLogInfo(s_log, "allowing the device to go to sleep");
			CompleteActivity();
		} else
			MojLogInfo(
					s_log,
					"done, but still waiting for device to sleep: %s (users left: %i)",
					reason.c_str(), m_stayAwakeCount);
	}
}

void PowerManager::CreateActivity()
{
	if (m_state == State_NoActivity) {
		MojString desc;
		MojErr err = desc.format("activity used to keep the device awake");
		ErrorToException(err);

		MojString name;
		err = name.format("Mail Power Activity %d", s_powerActivityCount++);
		ErrorToException(err);

		ActivityBuilder ab;
		ab.SetName(name.data());
		ab.SetDescription(desc);
		ab.SetForeground(true);
		ab.SetPowerOptions(true, false); // keep awake, but don't debounce

		m_activity = Activity::PrepareNewActivity(ab);
		m_activity->SetSlots(m_activityUpdateSlot, m_activityErrorSlot);
		m_activity->Create(m_client);
	}

	m_state = State_PendingCreation;
}

void PowerManager::CompleteActivity()
{
	if (m_state == State_Active) {
		m_activity->Complete(m_client);
		m_activityUpdateSlot.cancel();
		m_activityErrorSlot.cancel();
		m_activity.reset();
		m_state = State_NoActivity;
	} else if (m_state == State_PendingCreation) {
		m_state = State_PendingCompletion;
	}
}

MojErr PowerManager::ActivityUpdate(Activity* activity, Activity::EventType event)
{
	if (event == Activity::StartEvent) {
		if (m_state == State_PendingCompletion) {
			m_state = State_Active;
			CompleteActivity();
		} else {
			m_state = State_Active;
		}
	}

	return MojErrNone;
}

MojErr PowerManager::ActivityException(Activity* activity, Activity::ErrorType errorType, const exception& exc)
{
	MojLogCritical(s_log, "error occurred when managing power activity: %s (type: %i)", exc.what(), errorType);
	return MojErrNone;
}

void PowerManager::Shutdown()
{
	m_enabled = false;
	m_stayAwakeCount = 0;

	m_activityUpdateSlot.cancel();
	m_activityErrorSlot.cancel();

	if(m_activity.get()) {
		m_activity->Unsubscribe();
		m_activity.reset();
	}
}
