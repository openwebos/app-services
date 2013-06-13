// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "data/DeletedEmailsCache.h"
#include "data/OldEmailsCache.h"
#include "data/UidCacheAdapter.h"
//#include "PopClient.h"

const char* const UidCacheAdapter::EMAILS_UID_CACHE_KIND 	= "com.palm.pop.uidcache:1";

const char* const UidCacheAdapter::KIND		 				= "_kind";
const char* const UidCacheAdapter::ID		 				= "_id";
const char* const UidCacheAdapter::REV		 				= "_rev";
const char* const UidCacheAdapter::ACCOUNT_ID	 			= "accountId";
const char* const UidCacheAdapter::UID_CACHE_STRING			= "uidCache";
const char* const UidCacheAdapter::DELETED_EMAILS_CACHE		= "deletedEmails";
const char* const UidCacheAdapter::LOCAL_DELETED_EMAILS_CACHE		= "localDeleted";
const char* const UidCacheAdapter::PENDING_DELETED_EMAILS_CACHE		= "pendingDeleted";
const char* const UidCacheAdapter::OLD_EMAILS_CACHE			= "oldEmails";

// Get data from MojoDB and turn them into UidCache
void UidCacheAdapter::ParseDatabasePopObject(const MojObject& obj, UidCache& cache)
{
	MojErr err;

	if(obj.contains(ID)) {
		MojObject id;
		err = obj.getRequired(ID, id);
		ErrorToException(err);
		cache.SetId(id);
	}

	MojObject accountId;
	err = obj.getRequired(ACCOUNT_ID, accountId);
	ErrorToException(err);
	cache.SetAccountId(accountId);

	MojObject rev;
	err = obj.getRequired(REV, rev);
	ErrorToException(err);
	cache.SetRev(rev);

	MojString uidCacheString;
	err = obj.getRequired(UID_CACHE_STRING, uidCacheString);
	ErrorToException(err);

	ParseUidCacheString(uidCacheString, cache);
}

// Convert UidCache data into MojoDB.
void UidCacheAdapter::SerializeToDatabasePopObject(UidCache& cache, MojObject& obj)
{
	MojErr err;
	// Set the object kind
	err = obj.putString(KIND, EMAILS_UID_CACHE_KIND);
	ErrorToException(err);

	if(!cache.GetId().undefined()) {
		err = obj.put(ID, cache.GetId());
		ErrorToException(err);
	}

	// Set account id
	err = obj.put(ACCOUNT_ID, cache.GetAccountId());
	ErrorToException(err);

	// Set cache string
	MojString cacheStr;
	SerializeCacheMapToString(cache, cacheStr);
	err = obj.putString(UID_CACHE_STRING, cacheStr);
	ErrorToException(err);
}

/**
 * Converts the string representation of uid cache in MojoDB into UidCache object.
 */
void UidCacheAdapter::ParseUidCacheString(const MojString cacheStr, UidCache& cache)
{
	if (cacheStr == "") {
		return;
	}

	MojErr err;
	MojObject mojCache;
	err = mojCache.fromJson(cacheStr.data());
	ErrorToException(err);

	// Get deleted emails cache
	MojObject deletedEmails;
	if (mojCache.get(DELETED_EMAILS_CACHE, deletedEmails)) {
		MojObject localDeleted;
		MojObject pendingDeleted;

		if (deletedEmails.get(LOCAL_DELETED_EMAILS_CACHE, localDeleted)) {
			MojObject::ConstArrayIterator itr = localDeleted.arrayBegin();
			while (itr != localDeleted.arrayEnd()) {
				const MojObject& cacheEntry = *itr;
				itr++;

				MojString uid;
				err = cacheEntry.getRequired("uid", uid);
				ErrorToException(err);
				cache.GetDeletedEmailsCache().AddToLocalDeletedEmailCache(uid.data());
			}
		}

		if (deletedEmails.get(PENDING_DELETED_EMAILS_CACHE, pendingDeleted)) {
			MojObject::ConstArrayIterator itr = pendingDeleted.arrayBegin();
			while (itr != pendingDeleted.arrayEnd()) {
				const MojObject& cacheEntry = *itr;
				itr++;

				MojString uid;
				err = cacheEntry.getRequired("uid", uid);
				ErrorToException(err);
				cache.GetDeletedEmailsCache().AddToPendingDeletedEmailCache(uid.data());
			}
		}
	}

	// Get old emails cache
	MojObject oldEmailsArr;
	if (mojCache.get(OLD_EMAILS_CACHE, oldEmailsArr)) {
		MojObject::ConstArrayIterator itr = oldEmailsArr.arrayBegin();
		while (itr != oldEmailsArr.arrayEnd()) {
			const MojObject& cacheEntry = *itr;

			MojString uid;
			err = cacheEntry.getRequired("uid", uid);
			ErrorToException(err);

			MojInt64 timestamp;
			err = cacheEntry.getRequired("timestamp", timestamp);
			ErrorToException(err);

			cache.GetOldEmailsCache().AddToCache(uid.data(), timestamp);

			itr++;
		}
	}
}

/**
 * Converts the UidCache object into a string that will be persisted into MojoDB.
 */
void UidCacheAdapter::SerializeCacheMapToString(UidCache& cache, MojString& cacheStr)
{
	// Storing separate cache entry objects into MojoDB will be hard to manage.
	// So the uid cache of each POP account is converted into one signle string
	// that will be stored in MojoDB.
	MojObject mojCache;

	// Mojo object stores all the deleted email cache.
	MojObject mojDeletedCache;

	// first determine all the local deleted emails.
	DeletedEmailsCache delCache = cache.GetDeletedEmailsCache();
	DeletedEmailsCache::CacheSet uidSet = delCache.m_localDeletedCache;
	MojObject setArr;
	MojErr err;
	for (DeletedEmailsCache::CacheSet::iterator itr = uidSet.begin(); itr != uidSet.end(); itr++) {
		MojObject cacheEntry;
		err = cacheEntry.putString("uid", (*itr).c_str());
		ErrorToException(err);

		err = setArr.push(cacheEntry);
		ErrorToException(err);
	}
	err = mojDeletedCache.put(LOCAL_DELETED_EMAILS_CACHE, setArr);
	ErrorToException(err);

	// secondly determine all the pending deleted emails.
	uidSet = delCache.m_pendingDeletedCache;
	setArr.clear();
	for (DeletedEmailsCache::CacheSet::iterator itr = uidSet.begin(); itr != uidSet.end(); itr++) {
		MojObject cacheEntry;
		err = cacheEntry.putString("uid", (*itr).c_str());
		ErrorToException(err);

		err = setArr.push(cacheEntry);
		ErrorToException(err);
	}
	err = mojDeletedCache.put(PENDING_DELETED_EMAILS_CACHE, setArr);
	ErrorToException(err);

	// put all the deleted email cache into cache mojo object.
	err = mojCache.put(DELETED_EMAILS_CACHE, mojDeletedCache);
	ErrorToException(err);

	OldEmailsCache oldCache = cache.GetOldEmailsCache();
	OldEmailsCache::CacheMap uidMap = oldCache.m_uidCache;
	MojObject mapArr;
	for (OldEmailsCache::CacheMap::iterator itr = uidMap.begin(); itr != uidMap.end(); itr++) {
		// create a cache entry to hold uid and timestamp
		MojObject cacheEntry;
		err = cacheEntry.putString("uid", itr->first.c_str());
		ErrorToException(err);
		err = cacheEntry.putInt("timestamp", itr->second);
		ErrorToException(err);

		// add this cache entry into a mojo array
		err = mapArr.push(cacheEntry);
		ErrorToException(err);
	}
	err = mojCache.put(OLD_EMAILS_CACHE, mapArr);
	ErrorToException(err);

	// convert the mojo array into a mojo string
	err = mojCache.toJson(cacheStr);
	ErrorToException(err);
}
