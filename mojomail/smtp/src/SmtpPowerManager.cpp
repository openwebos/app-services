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

#include "activity/ActivityBuilder.h"
#include "SmtpPowerManager.h"
#include "SmtpClient.h"

using namespace std;

MojLogger& SmtpPowerManager::s_log = SmtpClient::s_log;

int SmtpPowerManager::s_powerActivityCount = 1;

SmtpPowerManager::SmtpPowerManager(SmtpClient& client)
 : m_client(client),
   m_stayAwakeCount(0),
   m_enabled(true),
   m_state(State_NoActivity),
   m_activityUpdateSlot(this, &SmtpPowerManager::ActivityUpdate),
   m_activityErrorSlot(this, &SmtpPowerManager::ActivityException)
{

}

SmtpPowerManager::~SmtpPowerManager()
{
	// destruction will clear m_activity, tearing down subscription, releasing activity.
}

void SmtpPowerManager::StayAwake(bool enabled, string reason)
{
	if (m_enabled) {
		if (enabled) {
			m_stayAwakeCount++;
			MojLogInfo(s_log, "keeping the device awake: %s (commands: %i)", reason.c_str(), m_stayAwakeCount);

			if (m_stayAwakeCount == 1)
				CreateActivity();
		} else {
			m_stayAwakeCount--;

			if (m_stayAwakeCount < 0)
				throw new MailException("unmatched power request", __FILE__, __LINE__);
			else if (m_stayAwakeCount == 0) {
				MojLogInfo(s_log, "allowing the device to go to sleep");
				CompleteActivity();
			} else
				MojLogInfo(s_log, "done, but still waiting for device to sleep: %s (command left: %i)", reason.c_str(), m_stayAwakeCount);
		}
	}
}

// TODO: USe activity factory and ActivitySet.
void SmtpPowerManager::CreateActivity()
{
	if (m_state == State_NoActivity) {
		MojString desc;
		MojErr err = desc.format("activity used to keep the device awake during an SMTP Outbox sync");
		ErrorToException(err);

		MojString name;
		err = name.format("SMTP Power Activity %d", s_powerActivityCount++);
		ErrorToException(err);

		ActivityBuilder ab;
		ab.SetName(name.data());
		ab.SetDescription(desc);
		ab.SetImmediate(true, ActivityBuilder::PRIORITY_LOW);
		ab.SetPowerOptions(true, true);

		m_activity = Activity::PrepareNewActivity(ab);
		m_activity->SetSlots(m_activityUpdateSlot, m_activityErrorSlot);
		m_activity->Create(m_client);
	}

	m_state = State_PendingCreation;
}

void SmtpPowerManager::CompleteActivity()
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

MojErr SmtpPowerManager::ActivityUpdate(Activity* activity, Activity::EventType event)
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

MojErr SmtpPowerManager::ActivityException(Activity* activity, Activity::ErrorType errorType, const exception& exc)
{
	MojLogCritical(s_log, "error occurred when managing power activity: %s (type: %i)", exc.what(), errorType);
	return MojErrNone;
}

