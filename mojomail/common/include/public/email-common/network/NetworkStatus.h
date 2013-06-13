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

#ifndef NETWORKSTATUS_H_
#define NETWORKSTATUS_H_

#include <activity/Activity.h>
#include <boost/shared_ptr.hpp>
#include <string>
#include "core/MojObject.h"

class InterfaceStatus
{
public:
	InterfaceStatus();
	virtual ~InterfaceStatus();

	typedef enum {
		UNKNOWN,
		POOR,
		FAIR,
		EXCELLENT
	} NetworkConfidence;

	static boost::shared_ptr<InterfaceStatus> ParseInterfaceStatus(const MojObject& status);

	bool IsConnected() const { return m_connected; }
	bool IsWakeOnWifiEnabled() const { return m_wakeOnWifiEnabled; }

	const std::string& GetIpAddress() const { return m_ipAddress; }

	NetworkConfidence GetNetworkConfidence() const { return m_networkConfidence; }

	// Compare to another InterfaceStatus. Make sure to deref pointers.
	bool operator==(const InterfaceStatus& other) const;

	void Status(MojObject& status) const;

protected:
	void ParseStatus(const MojObject& status);

	bool				m_connected;
	bool				m_wakeOnWifiEnabled;
	std::string			m_ipAddress;
	NetworkConfidence	m_networkConfidence;
};

class NetworkStatus
{
public:
	NetworkStatus();
	virtual ~NetworkStatus();

	// Reset the state of a NetworkStatus
	void Clear();

	// Attempt to parse a bus message payload, to see whether it has an activity
	// record with network requirement information, and update this object if it does.
	// Returns whether it found any information (which may be in either state).
	bool ParseMessagePayload(const MojObject& payload);
	
	// Parse network requirement status out of an existing Activity
	bool ParseActivity(const MojRefCountedPtr<Activity>& activity);
	
	// Parse an activity info object, to see whether it has network requirement information.
	bool ParseActivityInfo(const MojObject& info);
	
	// Parse network status
	void ParseStatus(const MojObject& status);

	// Whether we have information about whether the network is connected.
	bool IsKnown() const { return m_known; }

	// Whether the network is connected.
	bool IsConnected() const { return m_connected; }

	// Returns the best persistent interface.
	// May be a NULL pointer if no interface is persistent.
	const boost::shared_ptr<InterfaceStatus>& GetPersistentInterface() const;

	// Get WAN status
	const boost::shared_ptr<InterfaceStatus>& GetWanStatus() const { return m_wan; }

	// Get WIFI status
	const boost::shared_ptr<InterfaceStatus>& GetWifiStatus() const { return m_wifi; }

	// Compare to another NetworkStatus. Make sure to deref pointers.
	bool operator==(const NetworkStatus& other) const;

	void Status(MojObject& status) const;

protected:
	bool m_known;
	bool m_connected;

	boost::shared_ptr<InterfaceStatus>		m_wan;
	boost::shared_ptr<InterfaceStatus>		m_wifi;

	const boost::shared_ptr<InterfaceStatus> s_nullInterface;
};

#endif /* NETWORKSTATUS_H_ */
