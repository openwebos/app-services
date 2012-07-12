// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "data/OldEmailsCache.h"
//#include "PopClient.h"
//#include "PopDefs.h"

OldEmailsCache::OldEmailsCache()
: m_uidCache(),
  m_oldestEmailTimestamp(MojInt64Max),
  m_hasChanged(false)
{
}

OldEmailsCache::~OldEmailsCache()
{
}

/**
 * Adds an email, whose timestamp is beyond POP account's sync window, to
 * the cache.
 */
void OldEmailsCache::AddToCache(const std::string& uid, const MojInt64& timestamp)
{
	if (GetCacheSize() >= OldEmailsCache::CACHE_SIZE_LIMIT) {
		if (timestamp < m_oldestEmailTimestamp) {
			// if the timestmap is older than the oldest timestamp in the cache,
			// there is no reason to keep this new uid-timestamp in the cache.
			return;
		}

		MakeRoomForNewItems();
	}

	m_uidCache[uid] = timestamp;

	if (m_oldestEmailTimestamp > timestamp) {
		m_oldestEmailTimestamp = timestamp;
	}
}

/**
 * Adds an email, whose timestamp is beyond POP account's sync window, to
 * the cache.
 */
void OldEmailsCache::AddEmailToCache(const PopEmail& email)
{
	std::string uid = email.GetServerUID();
	MojInt64 timestamp = email.GetDateReceived();
	AddToCache(uid, timestamp);
}

/**
 * Uses the UID of an email to look up its timestamp from the cache.  If the
 * UID doesn't exist in the cache, MojInt64Max will be returned.  Otherwise,
 * the cached timestamp will be returned.  The boolean input 'found' will be
	 * set to true if the UID is found in the cache or false otherwise.
 */
const MojInt64 OldEmailsCache::GetEmailTimestamp(const std::string uid) const
{
	CacheMap::const_iterator itr = m_uidCache.find(uid);
	if (itr == m_uidCache.end()) {
		return MojInt64Max;
	} else {
		return itr->second;
	}
}

/**
 * Removes any old email cache whose timestamp is within the sync window.
 * This function is to handle the case that the sync window is changed from
 * 3 days to 7 days.  Then some of the "old emails" from previous sync is
 * now considered "new emails".
 */
void OldEmailsCache::UpdateCacheWithSyncWindow(const MojInt64 syncWindow)
{
	std::vector<std::string> uids;

	for (CacheMap::iterator mitr = m_uidCache.begin(); mitr != m_uidCache.end(); mitr++) {
		MojInt64 ts = mitr->second;
		if (ts > syncWindow) {
			uids.push_back(mitr->first);
		}
	}

	if (!uids.empty()) {
		SetChanged(true);
	}

	for (std::vector<std::string>::iterator vitr = uids.begin(); vitr != uids.end(); vitr++) {
		m_uidCache.erase(*vitr);
	}
}

/**
 * Removes an email, whose timestamp is beyond POP account's sync window,
 * from the cache. This function is used when the old email is removed from
 * POP server.
 */
void OldEmailsCache::RemoveEmailFromCache(const std::string uid)
{
	CacheMap::iterator itr = m_uidCache.find(uid);
	if (itr != m_uidCache.end()) {
		MojInt64 ts = itr->second;
		m_uidCache.erase(itr);

		if (ts == m_oldestEmailTimestamp) {
			// If the oldest email's UID is removed, we need to find the next
			// oldest email's timestamp and update 'm_oldestEmailTimestamp' value.
			RecalculateOldestEmailTimestamp();
		}
	}

	SetChanged(true);
}

/**
 * Find the oldest email's timestamp and update 'm_oldestEmailTimestamp' to
 * this value.
 */
void OldEmailsCache::RecalculateOldestEmailTimestamp()
{
	m_oldestEmailTimestamp = MojInt64Max;
	
	for (CacheMap::const_iterator itr = m_uidCache.begin(); itr != m_uidCache.end(); itr++) {
		MojInt64 ts = itr->second;
		if (m_oldestEmailTimestamp > ts) {
			m_oldestEmailTimestamp = ts;
		}
	}
}

/**
 * When the cache reaches its maximum capacity, adding a new UID-timestamp
 * pair will require the cache to reduce its size by ten percent first.  This
 * function will shrink the cache by removing one hundred UID-timestamp with
 * the oldest timestamp.
 */
void OldEmailsCache::MakeRoomForNewItems()
{
	CacheMap::iterator itr;
	OldestEmailsQueue queue;

	// Add all cache entries in a priority queue.  The top element of the queue
	// is always the cache entry with the latest timestamp.
	for (itr = m_uidCache.begin(); itr != m_uidCache.end(); itr++) {
		CacheEntry entry(itr->first, itr->second);
		queue.push(entry);
	}

	while (!queue.empty()) {
		// keep popping entries until there are only 100 cache entries left
		if ((int)queue.size() <= OldEmailsCache::QUEUE_SIZE_LIMIT) {
			// only remove the oldest 100 email cache entries
			CacheEntry entry = queue.top();

			// when UIDs are removed from the cache, RecalculateOldestEmailTimestamp()
			// function should be only involved once by RemoveEmailFromCache()
			// function since the oldest email's UID is the last one to be popped.
			RemoveEmailFromCache(entry.GetUid());
		}

		queue.pop();
	}
}

/**
 * Returns the size of the cache.
 */
int OldEmailsCache::GetCacheSize()
{
	return m_uidCache.size();
}
