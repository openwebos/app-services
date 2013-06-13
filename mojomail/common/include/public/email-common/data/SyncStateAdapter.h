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

#ifndef SYNCSTATEADAPTER_H_
#define SYNCSTATEADAPTER_H_

#include "core/MojObject.h"
#include "data/SyncState.h"

class SyncStateAdapter
{
public:
	static const char* const 	SYNC_STATE_KIND;
	static const char* const 	CAPABILITY_PROVIDER;
	static const char* const 	ACCOUNT_ID;
	static const char* const 	COLLECTION_ID;
	static const char* const 	METADATA;
	static const char* const 	BUS_ADDRESS;
	static const char* const 	SYNC_STATE;
	static const char* const 	ERROR_CODE;
	static const char* const 	ERROR_TEXT;

	static const char* const 	STATE_IDLE;
	static const char* const 	STATE_PUSH;
	static const char* const 	STATE_INITIAL_SYNC;
	static const char* const 	STATE_INCREMENTAL_SYNC;
	static const char* const 	STATE_DELETE;
	static const char* const 	STATE_ERROR;

	// Serialize sync state
	static void SerializeToDatabaseObject(const SyncState& syncState, MojObject& obj, const char* capabilityProvider, const char* busAddress);

private:
	SyncStateAdapter();
	virtual ~SyncStateAdapter();

	static const char* GetStateName(SyncState::State state);
};

#endif /* SYNCSTATEADAPTER_H_ */
