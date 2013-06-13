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

#ifndef SYNCSTATEUPDATER_H_
#define SYNCSTATEUPDATER_H_

#include <string>
#include "data/SyncState.h"
#include "db/MojDbClient.h"

class BusClient;

class SyncStateUpdater : public MojSignalHandler
{
public:
	SyncStateUpdater(BusClient& busClient, const char* capabilityProvider, const char* busAddress);

	virtual void UpdateSyncState(MojSignal<>::SlotRef slot, const SyncState& syncState, bool clearSyncState = false);

protected:
	virtual ~SyncStateUpdater();

	virtual MojErr UpdateSyncStateResponse(MojObject& response, MojErr err);

	BusClient&		m_busClient;
	std::string		m_capabilityProvider;
	std::string		m_busAddress;

	MojSignal<>		m_doneSignal;

	MojDbClient::Signal::Slot<SyncStateUpdater>		m_updateSlot;
};

#endif /* SYNCSTATEUPDATER_H_ */
