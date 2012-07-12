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

#include "data/SyncStateAdapter.h"
#include "data/DatabaseAdapter.h"
#include "data/SyncState.h"
#include "CommonPrivate.h"

const char* const SyncStateAdapter::SYNC_STATE_KIND 		= "com.palm.account.syncstate:1";
const char* const SyncStateAdapter::CAPABILITY_PROVIDER		= "capabilityProvider";
const char* const SyncStateAdapter::ACCOUNT_ID 				= "accountId";
const char* const SyncStateAdapter::COLLECTION_ID 			= "collectionId";
const char* const SyncStateAdapter::METADATA				= "metadata";
const char* const SyncStateAdapter::BUS_ADDRESS				= "busAddress";
const char* const SyncStateAdapter::SYNC_STATE				= "syncState";
const char* const SyncStateAdapter::ERROR_CODE				= "errorCode";
const char* const SyncStateAdapter::ERROR_TEXT				= "errorText";

const char* const SyncStateAdapter::STATE_IDLE				= "IDLE";
const char* const SyncStateAdapter::STATE_PUSH				= "PUSH";
const char* const SyncStateAdapter::STATE_INITIAL_SYNC		= "INITIAL_SYNC";
const char* const SyncStateAdapter::STATE_INCREMENTAL_SYNC	= "INCREMENTAL_SYNC";
const char* const SyncStateAdapter::STATE_DELETE			= "DELETE";
const char* const SyncStateAdapter::STATE_ERROR				= "ERROR";

const char* SyncStateAdapter::GetStateName(SyncState::State state)
{
	switch(state) {
	case SyncState::IDLE: return STATE_IDLE;
	case SyncState::PUSH: return STATE_PUSH;
	case SyncState::INITIAL_SYNC: return STATE_INITIAL_SYNC;
	case SyncState::INCREMENTAL_SYNC: return STATE_INCREMENTAL_SYNC;
	case SyncState::DELETE: return STATE_DELETE;
	case SyncState::ERROR: return STATE_ERROR;
	}
	throw MailException("invalid state", __FILE__, __LINE__);
}

void SyncStateAdapter::SerializeToDatabaseObject(const SyncState& syncState, MojObject& obj, const char* capabilityProvider, const char* busAddress)
{
	MojErr err;

	err = obj.putString(DatabaseAdapter::KIND, SyncStateAdapter::SYNC_STATE_KIND);
	ErrorToException(err);

	err = obj.putString(SyncStateAdapter::CAPABILITY_PROVIDER, capabilityProvider);
	ErrorToException(err);

	err = obj.putString(SyncStateAdapter::BUS_ADDRESS, busAddress);
	ErrorToException(err);

	err = obj.put(SyncStateAdapter::ACCOUNT_ID, syncState.GetAccountId());
	ErrorToException(err);

	err = obj.put(SyncStateAdapter::COLLECTION_ID, syncState.GetCollectionId());
	ErrorToException(err);

	err = obj.putString(SyncStateAdapter::SYNC_STATE, GetStateName(syncState.GetState()));
	ErrorToException(err);

	err = obj.putString(SyncStateAdapter::ERROR_CODE, syncState.GetErrorCode());
	ErrorToException(err);

	err = obj.putString(SyncStateAdapter::ERROR_TEXT, syncState.GetErrorText());
	ErrorToException(err);

	// These fields are not currently used, but let's clear them out
	MojObject null(MojObject::TypeNull);

	err = obj.put(SyncStateAdapter::METADATA, null);
	ErrorToException(err);

}
