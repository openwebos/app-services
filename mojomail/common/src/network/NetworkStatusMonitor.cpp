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

#include "network/NetworkStatusMonitor.h"
#include "activity/ActivityBuilder.h"
#include "CommonPrivate.h"
#include "client/BusClient.h"

using namespace std;

NetworkStatusMonitor::NetworkStatusMonitor(BusClient& busClient)
: m_busClient(busClient),
  m_isSubscribed(false),
  m_lastUpdateTime(0),
  m_updateSignal(this),
  m_activityUpdateSlot(this, &NetworkStatusMonitor::ActivityUpdated),
  m_activityErrorSlot(this, &NetworkStatusMonitor::ActivityError)
{
}

NetworkStatusMonitor::~NetworkStatusMonitor()
{
}

bool NetworkStatusMonitor::HasCurrentStatus()
{
	return m_networkStatus.IsKnown();
}

const NetworkStatus& NetworkStatusMonitor::GetCurrentStatus() const
{
	return m_networkStatus;
}

void NetworkStatusMonitor::WaitForStatus(MojSignal<>::SlotRef slot)
{
	slot.cancel(); // just in case

	if(m_networkStatus.IsKnown()) {
		MojSignal<> signal(this);
		signal.connect(slot);
		signal.fire();
	} else {
		m_updateSignal.connect(slot);

		if(!m_isSubscribed && m_activity.get() == NULL) {
			CreateActivity();
		}
	}
}

void NetworkStatusMonitor::SetFakeStatus(MojObject& fakeStatus, bool persist)
{
	m_networkStatus.ParseStatus(fakeStatus);
	m_lastUpdateTime = time(NULL);

	if(persist) {
		m_isSubscribed = true;

		if(m_activity.get()) {
			m_activity->Unsubscribe();
			m_activity.reset();
		}
	} else {
		m_isSubscribed = false;
	}
}

void NetworkStatusMonitor::CreateActivity()
{
	ActivityBuilder ab;

	static MojInt64 uniqueId = 0;

	string serviceName = m_busClient.GetServiceName();

	// FIXME
	if(serviceName.empty()) {
		throw MailException("no service name", __FILE__, __LINE__);
	}

	MojString name;
	MojErr err = name.format("%s network status check - %lld", serviceName.c_str(), ++uniqueId);
	ErrorToException(err);

	ab.SetName(name.data());
	ab.SetDescription("Monitors network status");
	ab.SetExplicit(false);
	ab.SetPersist(false);
	ab.SetForeground(true);
	ab.SetRequiresInternet(true);

	m_activity = Activity::PrepareNewActivity(ab, true, true);
	m_activity->SetSlots(m_activityUpdateSlot, m_activityErrorSlot);
	m_activity->Create(m_busClient);
}

MojErr NetworkStatusMonitor::ActivityUpdated(Activity* activity, Activity::EventType event)
{
	if(event == Activity::StartEvent || event == Activity::UpdateEvent) {
		//fprintf(stderr, "updating network status from activity");
		m_networkStatus.ParseActivity(m_activity);
		m_lastUpdateTime = time(NULL);

		m_isSubscribed = true;

		if(m_networkStatus.IsKnown()) {
			m_updateSignal.fire();
		}
	}

	return MojErrNone;
}

MojErr NetworkStatusMonitor::ActivityError(Activity* activity, Activity::ErrorType event, const exception& e)
{
	m_isSubscribed = false;
	m_activity.reset();

	m_updateSignal.fire();

	return MojErrNone;
}

void NetworkStatusMonitor::Status(MojObject& status) const
{
	MojErr err;

	if(m_lastUpdateTime) {
		MojObject networkStatus;
		m_networkStatus.Status(networkStatus);

		err = status.put("networkStatus", networkStatus);
		ErrorToException(err);
	}

	err = status.put("lastUpdateTime", (MojInt64) m_lastUpdateTime);
	ErrorToException(err);
}
