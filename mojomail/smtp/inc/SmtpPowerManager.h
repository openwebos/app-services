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

#ifndef SMTPPOWERMANAGER_H_
#define SMTPPOWERMANAGER_H_

#include "activity/Activity.h"
#include "PowerManager.h"

class SmtpClient;

class SmtpPowerManager : public PowerManager, public MojSignalHandler
{
public:
	SmtpPowerManager(SmtpClient& client);
	virtual ~SmtpPowerManager();

	void enabled(bool enabled) { m_enabled = enabled; };
	virtual void StayAwake(bool enabled, std::string reason = "");

private:
	typedef enum {
		State_NoActivity,
		State_PendingCreation,
		State_Active,
		State_PendingCompletion
	} State;

	static MojLogger& s_log;

	void CreateActivity();
	void CompleteActivity();
	MojErr ActivityUpdate(Activity* activity, Activity::EventType event);
	MojErr ActivityException(Activity* activity, Activity::ErrorType errorType, const std::exception& exc);

	SmtpClient& m_client;

	/**
	 * Number of remaining requests left to keep the device awake.
	 */
	int m_stayAwakeCount;

	bool m_enabled;

	/**
	 * The object which represents the power activity in the ActivityManager.
	 */
	MojRefCountedPtr<Activity> m_activity;

	State m_state;

	Activity::UpdateSignal::Slot<SmtpPowerManager>	m_activityUpdateSlot;
	Activity::ErrorSignal::Slot<SmtpPowerManager>	m_activityErrorSlot;
	
	static int s_powerActivityCount;
};

#endif /* SMTPPOWERMANAGER_H_ */
