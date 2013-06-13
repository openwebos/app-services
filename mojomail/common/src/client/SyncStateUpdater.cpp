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

#include "client/SyncStateUpdater.h"
#include "client/BusClient.h"
#include "data/SyncStateAdapter.h"
#include "db/MojDbQuery.h"
#include "CommonPrivate.h"

SyncStateUpdater::SyncStateUpdater(BusClient& busClient, const char* capabilityProvider, const char* busAddress)
: m_busClient(busClient), m_capabilityProvider(capabilityProvider), m_busAddress(busAddress),
  m_doneSignal(this),
  m_updateSlot(this, &SyncStateUpdater::UpdateSyncStateResponse)
{
}

SyncStateUpdater::~SyncStateUpdater()
{
}

void SyncStateUpdater::UpdateSyncState(MojSignal<>::SlotRef slot, const SyncState& syncState, bool clearSyncState)
{
	m_doneSignal.connect(slot);

	MojErr err;
	MojObject batchPayload;
	MojObject batchOperations;

	// Delete old sync state

	MojObject delPayload;

	MojDbQuery query;

	err = query.from(SyncStateAdapter::SYNC_STATE_KIND);
	ErrorToException(err);

	MojString capabilityProvider;
	capabilityProvider.assign(m_capabilityProvider.c_str());
	err = query.where(SyncStateAdapter::CAPABILITY_PROVIDER, MojDbQuery::OpEq, capabilityProvider);
	ErrorToException(err);

	err = query.where(SyncStateAdapter::ACCOUNT_ID, MojDbQuery::OpEq, syncState.GetAccountId());
	ErrorToException(err);

	err = query.where(SyncStateAdapter::COLLECTION_ID, MojDbQuery::OpEq, syncState.GetCollectionId());
	ErrorToException(err);

	MojObject queryObj;
	err = query.toObject(queryObj);
	ErrorToException(err);

	err = delPayload.put("query", queryObj);
	ErrorToException(err);

	MojObject delOperation;
	err = delOperation.putString("method", "del");
	ErrorToException(err);
	err = delOperation.put("params", delPayload);
	ErrorToException(err);

	err = batchOperations.push(delOperation);
	ErrorToException(err);

	if(!clearSyncState) { // Store new sync state
		MojObject putPayload;

		MojObject syncStateObj;
		SyncStateAdapter::SerializeToDatabaseObject(syncState, syncStateObj, m_capabilityProvider.c_str(), m_busAddress.c_str());

		MojObject objects;
		err = objects.push(syncStateObj);
		ErrorToException(err);

		err = putPayload.put("objects", objects);
		ErrorToException(err);

		MojObject putOperation;
		err = putOperation.putString("method", "put");
		ErrorToException(err);
		err = putOperation.put("params", putPayload);
		ErrorToException(err);

		err = batchOperations.push(putOperation);
		ErrorToException(err);
	}

	err = batchPayload.put("operations", batchOperations);
	ErrorToException(err);

	// Note: batch operations are not atomic, this is just for convenience and performance
	m_busClient.SendRequest(m_updateSlot, "com.palm.tempdb", "batch", batchPayload);
}

MojErr SyncStateUpdater::UpdateSyncStateResponse(MojObject& response, MojErr err)
{
	// ignore errors
	m_doneSignal.fire();

	return MojErrNone;
}
