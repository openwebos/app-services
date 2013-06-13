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

#ifndef NETWORKSTATUSMONITOR_H_
#define NETWORKSTATUSMONITOR_H_

#include "core/MojSignal.h"
#include "activity/NetworkStatus.h"
#include "activity/Activity.h"

class NetworkStatusMonitor : public MojSignalHandler
{
public:
	NetworkStatusMonitor(BusClient& busClient);
	virtual ~NetworkStatusMonitor();

	/**
	 * Returns true if the network status is up to date.
	 * This is only true if we're currently monitoring an activity.
	 */
	bool HasCurrentStatus();

	/**
	 * Returns the current network status.
	 */
	const NetworkStatus& GetCurrentStatus() const;

	/**
	 * Wait for latest network status.
	 */
	void WaitForStatus(MojSignal<>::SlotRef slot);

	/**
	 * Status.
	 */
	void Status(MojObject& status) const;

	/**
	 * For debugging only
	 *
	 * If persist is set to true, disable normal updates (not implemented yet)
	 */
	void SetFakeStatus(MojObject& status, bool persist);

protected:
	void CreateActivity();
	MojErr ActivityUpdated(Activity* activity, Activity::EventType event);
	MojErr ActivityError(Activity* activity, Activity::ErrorType errorType, const std::exception& e);

	BusClient&		m_busClient;
	std::string		m_serviceName;

	NetworkStatus	m_networkStatus;

	ActivityPtr		m_activity;
	bool			m_isSubscribed; // currently subscribed to ActivityManager updates
	time_t			m_lastUpdateTime;

	MojSignal<>		m_updateSignal;

	Activity::UpdateSignal::Slot<NetworkStatusMonitor>		m_activityUpdateSlot;
	Activity::ErrorSignal::Slot<NetworkStatusMonitor>		m_activityErrorSlot;
};

#endif /* NETWORKSTATUSMONITOR_H_ */
