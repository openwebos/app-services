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

#ifndef POWERMANAGER_H_
#define POWERMANAGER_H_

#include "activity/Activity.h"
#include <string>

class BusClient;

class PowerManager : public MojSignalHandler
{
public:
	PowerManager(BusClient& client);
	virtual ~PowerManager();

	/**
	 * Enable or disable the power manager.
	 *
	 * If the power manager is disabled, calls to StayAwake will be ignored.
	 */
	void SetEnabled(bool enable) { m_enabled = enable; }

	/**
	 * Returns whether the power manager is enabled or not.
	 */
	bool IsEnabled() const { return m_enabled; }

	/**
	 * Keep the device awake. Call StayAwake with enabled = true to keep the power on,
	 * which must be matched with a call to StayAwake with enabled = false later.
	 */
	void StayAwake(bool enabled, std::string reason = "");

	// Kill power activity without waiting for any responses.
	// This should be called to prevent BusClient methods from getting called from a destructor,
	// e.g. early in a client destructor.
	void Shutdown();

	static MojLogger s_log;

private:
	typedef enum {
		State_NoActivity,
		State_PendingCreation,
		State_Active,
		State_PendingCompletion
	} State;

	void CreateActivity();
	void CompleteActivity();
	MojErr ActivityUpdate(Activity* activity, Activity::EventType event);
	MojErr ActivityException(Activity* activity, Activity::ErrorType errorType, const std::exception& exc);

	BusClient& m_client;

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

	Activity::UpdateSignal::Slot<PowerManager>	m_activityUpdateSlot;
	Activity::ErrorSignal::Slot<PowerManager>	m_activityErrorSlot;

	static int s_powerActivityCount;
};

#endif /* POWERMANAGER_H_ */
