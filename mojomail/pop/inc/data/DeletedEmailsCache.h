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

#ifndef DELETEDEMAILSCACHE_H_
#define DELETEDEMAILSCACHE_H_

#include <set>
#include <string>
#include "core/MojObject.h"

/**
 * A class that holds the UIDs of the locally and pending deleted emails for a POP account
 * that is configured to have "Sync deleted emails" turned off.  If this flag is
 * off and we don't have a place to store the locally deleted emails, the deleted
 * emails will be added back during inbox sync.  So we use this class to accomplish
 * this feature.
 */
class DeletedEmailsCache {
public:
	typedef std::set<std::string> 	CacheSet;

	DeletedEmailsCache();
	virtual ~DeletedEmailsCache();

	/**
	 * Adds an email's UID into locally deleted emails' cache.
	 */
	void AddToLocalDeletedEmailCache(const std::string& uid) { m_localDeletedCache.insert(uid); }

	/**
	 * Gets the set of local deleted emails.
	 */
	CacheSet& GetLocalDeletedCache() { return m_localDeletedCache; }

	/**
	 * Removes all entries in locally deleted email cache.  This is the case
	 * when the "Sync deleted emails" is set to true.  Then we don't need this
	 * cache any more.
	 */
	void ClearLocalDeletedEmailCache();

	/**
	 * Adds an email's UID into pending deleted emails' cache.
	 */
	void AddToPendingDeletedEmailCache(const std::string& uid) { m_pendingDeletedCache.insert(uid); }

	/**
	 * Gets the set of pending deleted emails.
	 */
	CacheSet& GetPendingDeletedCache() 		{ return m_pendingDeletedCache; }

	/**
	 * Removes all entries in pending deleted email cache.  This should be used
	 * after all the pending deleted emails are handled.
	 */
	void ClearPendingDeletedEmailCache();

	/**
	 * Returns true if this cache has any items added or deleted.
	 */
	bool HasChanged() { return m_hasChanged; }

	void SetChanged(bool changed)	{ if (changed != m_hasChanged) { m_hasChanged = changed; } }
private:
	CacheSet 		m_localDeletedCache;
	CacheSet 		m_pendingDeletedCache;
	bool			m_hasChanged;

	friend class UidCacheAdapter;
};

#endif /* DELETEDEMAILSCACHE_H_ */
