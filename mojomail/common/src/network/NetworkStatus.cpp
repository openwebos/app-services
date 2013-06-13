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

#include "activity/NetworkStatus.h"
#include "CommonPrivate.h"

InterfaceStatus::InterfaceStatus()
: m_connected(false), m_wakeOnWifiEnabled(true), m_networkConfidence(UNKNOWN)
{
}

InterfaceStatus::~InterfaceStatus()
{
}

boost::shared_ptr<InterfaceStatus> InterfaceStatus::ParseInterfaceStatus(const MojObject& status)
{
	boost::shared_ptr<InterfaceStatus> interfaceStatus = boost::make_shared<InterfaceStatus>();

	interfaceStatus->ParseStatus(status);

	return interfaceStatus;
}

void InterfaceStatus::ParseStatus(const MojObject& status)
{
	MojErr err;

	MojString state;
	err = status.getRequired("state", state);
	ErrorToException(err);

	if(state == "connected") {
		m_connected = true;
	}

	bool wakeOnWifiEnabled = false;
	if(status.get("isWakeOnWifiEnabled", wakeOnWifiEnabled)) {
		m_wakeOnWifiEnabled = wakeOnWifiEnabled;
	} else if(status.get("isPersistent", wakeOnWifiEnabled)) {
		// FIXME: backwards compatibility
		m_wakeOnWifiEnabled = wakeOnWifiEnabled;
	}

	MojString ipAddress;
	bool hasIpAddress = false;
	err = status.get("ipAddress", ipAddress, hasIpAddress);
	ErrorToException(err);

	if(hasIpAddress) {
		m_ipAddress.assign(ipAddress.data());
	}

	bool hasNetworkConfidence = false;
	MojString networkConfidenceLevel;
	err = status.get("networkConfidenceLevel", networkConfidenceLevel, hasNetworkConfidence);
	if (hasNetworkConfidence) {
		if (networkConfidenceLevel == "excellent")
			m_networkConfidence = EXCELLENT;
		else if (networkConfidenceLevel == "fair")
			m_networkConfidence = FAIR;
		else if (networkConfidenceLevel == "poor")
			m_networkConfidence = POOR;
		else
			m_networkConfidence = UNKNOWN;
	}
}

bool InterfaceStatus::operator==(const InterfaceStatus& other) const
{
	return m_connected == other.m_connected
		&& m_wakeOnWifiEnabled == other.m_wakeOnWifiEnabled
		&& m_ipAddress == other.m_ipAddress
		&& m_networkConfidence == other.m_networkConfidence;
}

void InterfaceStatus::Status(MojObject& status) const
{
	MojErr err;

	err = status.put("connected", m_connected);
	ErrorToException(err);

	if(m_connected) {
		err = status.put("networkConfidence", m_networkConfidence);
		ErrorToException(err);
	}
}

NetworkStatus::NetworkStatus()
: m_known(false),
  m_connected(false),
  m_wan(new InterfaceStatus()),
  m_wifi(new InterfaceStatus())
{
}

NetworkStatus::~NetworkStatus()
{
}

bool NetworkStatus::ParseMessagePayload(const MojObject& payload)
{
	MojObject activityState;
	
	if (payload.get("$activity", activityState)) {
		return ParseActivityInfo(activityState);
	}
	return false;
}

bool NetworkStatus::ParseActivityInfo(const MojObject& info)
{
	MojObject requirements;
	if (info.get("requirements", requirements)) {
		MojObject internet;
		if (requirements.get("internet", internet)) {
			ParseStatus(internet);
			return true;
		}
	}
	return false;
}

bool NetworkStatus::ParseActivity(const MojRefCountedPtr<Activity>& activity)
{
	if (!activity.get())
		return false;
	
	const MojObject& info = activity->GetInfo();
	
	return ParseActivityInfo(info);
}

void NetworkStatus::ParseStatus(const MojObject& status)
{
	MojObject wanStatus, wifiStatus;

	m_known = true;

	bool connected = false;
	if(status.get("isInternetConnectionAvailable", connected)) {
		m_connected = connected;
	} else {
		m_connected = false;
	}

	if(status.get("wan", wanStatus)) {
		m_wan = InterfaceStatus::ParseInterfaceStatus(wanStatus);
	} else {
		m_wan.reset( new InterfaceStatus() );
	}

	if(status.get("wifi", wifiStatus)) {
		m_wifi = InterfaceStatus::ParseInterfaceStatus(wifiStatus);
	} else {
		m_wifi.reset( new InterfaceStatus() );
	}
	
}

void NetworkStatus::Clear()
{
	m_known = false;
	m_connected = false;
	m_wan.reset( new InterfaceStatus() );
	m_wifi.reset( new InterfaceStatus() );
} 

const boost::shared_ptr<InterfaceStatus>& NetworkStatus::GetPersistentInterface() const
{
	if(m_wifi->IsWakeOnWifiEnabled() && m_wifi->GetNetworkConfidence() >= InterfaceStatus::FAIR)
		return m_wifi;
	else if(m_wan->GetNetworkConfidence() >= InterfaceStatus::FAIR)
		return m_wan;
	else
		return s_nullInterface; // equivalent to boost::shared_ptr<InterfaceStatus>(NULL)
}

bool NetworkStatus::operator==(const NetworkStatus& other) const
{
	return m_known == other.m_known
		&& m_connected == other.m_connected
		&& *m_wan == *(other.m_wan)
		&& *m_wifi == *(other.m_wifi);
}

void NetworkStatus::Status(MojObject& status) const
{
	MojErr err;

	err = status.put("connected", m_connected);
	ErrorToException(err);

	if(m_wan.get()) {
		MojObject wanStatus;
		m_wan->Status(wanStatus);
		err = status.put("wan", wanStatus);
		ErrorToException(err);
	}

	if(m_wifi.get()) {
		MojObject wifiStatus;
		m_wifi->Status(wifiStatus);
		err = status.put("wifi", wifiStatus);
		ErrorToException(err);
	}
}
