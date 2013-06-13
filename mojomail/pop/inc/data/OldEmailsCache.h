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

#ifndef OLDEMAILSCACHE_H_
#define OLDEMAILSCACHE_H_

#include <map>
#include <queue>
#include <string>
#include <boost/shared_ptr.hpp>
#include "core/MojObject.h"
#include "data/PopEmail.h"
#include "data/PopAccount.h"

/**
 * A class that holds the cache of email's UID and its timestamp pair for older
 * emails.  The older emails are the ones whose timestamp is beyond the sync
 * window that is set for the POP account.
 *
 * This class is used to hold the UID and timestamp of the older emails that POP
 * transport has analyzed in the previous syncs.  With this cache, we can easily
 * look up past email's timestamp using email's UID.  If the email's UID is in
 * the cache, we can obtain the timestamp right away without the need to download
 * email's header again.  This cache object will improve performance and minimize
 * network usage.
 *
 */
class OldEmailsCache {
public:
	// The maximum entries that the cache can hold.
	static const int 							CACHE_SIZE_LIMIT = 1200;

	typedef std::map<std::string, MojInt64> 	CacheMap;

	OldEmailsCache();
	virtual ~OldEmailsCache();

	/**
	 * Adds an email's UID and timestamp pair into older emails' cache.
	 */
	void AddToCache(const std::string& uid, const MojInt64& timestamp);

	/**
	 * Adds an email, whose timestamp is beyond POP account's sync window, to
	 * the cache.
	 */
	void AddEmailToCache(const PopEmail& email);

	/**
	 * Uses the UID of an email to look up its timestamp from the cache.  If the
	 * UID doesn't exist in the cache, MojInt64Max will be returned.  Otherwise,
	 * the cached timestamp will be returned.  The boolean input 'found' will be
	 * set to true if the UID is found in the cache or false otherwise.
	 */
	const MojInt64 GetEmailTimestamp(const std::string uid) const;

	/**
	 * Removes any old email cache whose timestamp is beyond the sync window.
	 * This function is to handle the case that the sync window is changed from
	 * 3 days to 7 days.  Then some of the "old emails" from previous sync is
	 * now considered "new emails".
	 */
	void UpdateCacheWithSyncWindow(const MojInt64 syncWindow);

	/**
	 * Removes an email, whose timestamp is beyond POP account's sync window,
	 * from the cache. This function is used when the old email is removed from
	 * POP server.
	 */
	void RemoveEmailFromCache(const std::string uid);

	/**
	 * Returns the size of the cache.
	 */
	int GetCacheSize();

	CacheMap&	GetCacheMap()  { return m_uidCache; }

	/**
	 * Returns true if this cache has any items added or deleted.
	 */
	bool HasChanged()				{ return m_hasChanged; }

	void SetChanged(bool changed)	{ if (changed != m_hasChanged) { m_hasChanged = changed; } }

private:
	// The number of cache entries to be purged before a new cache entry can be
	// inserted into cache.
	static const int 							QUEUE_SIZE_LIMIT = 100;

	class CacheEntry {
	public:
		CacheEntry(std::string uid, MojInt64 timestamp) : m_uid(uid), m_timestamp(timestamp) { }
		~CacheEntry() { }

		const std::string GetUid() const				{ return m_uid; }
		bool operator==(const CacheEntry& rhs) const 	{ return m_timestamp == rhs.m_timestamp; }
		bool operator!=(const CacheEntry& rhs) const 	{ return !operator==(rhs); }
		bool operator<(const CacheEntry& rhs) const 	{ return m_timestamp < rhs.m_timestamp; }
		bool operator<=(const CacheEntry& rhs) const 	{ return m_timestamp <= rhs.m_timestamp; }
		bool operator>(const CacheEntry& rhs) const 	{ return m_timestamp > rhs.m_timestamp; }
		bool operator>=(const CacheEntry& rhs) const 	{ return m_timestamp >= rhs.m_timestamp; }

	private:
		std::string m_uid;
		MojInt64 m_timestamp;
	};

	typedef std::priority_queue<CacheEntry> OldestEmailsQueue;

	/**
	 * Find the oldest email's timestamp and update 'm_oldestEmailTimestamp' to
	 * this value.
	 */
	void RecalculateOldestEmailTimestamp();

	/**
	 * When the cache reaches its maximum capacity, adding a new UID-timestamp
	 * pair will require the cache to reduce its size by ten percent first.  This
	 * function will shrink the cache by removing one hundred UID-timestamp with
	 * the oldest timestamp.
	 */
	void MakeRoomForNewItems();

	CacheMap 			m_uidCache;
	// the timestamp that represents the oldest email in the cache.
	MojInt64			m_oldestEmailTimestamp;
	bool				m_hasChanged;

	friend class UidCacheAdapter;
};

#endif /* OLDEMAILSCACHE_H_ */
