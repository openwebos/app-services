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

#ifndef UIDCACHE_H_
#define UIDCACHE_H_

#include "data/DeletedEmailsCache.h"
#include "data/OldEmailsCache.h"

class UidCache {
public:
	UidCache();
	virtual ~UidCache();

	// Setters
	/**
	 * Set MojoDB ID to this cache.
	 */
	void SetId(const MojObject& id)			{ m_id = id; }
	/**
	 * Set account ID to this cache.
	 */
	void SetAccountId(const MojObject& id) 	{ m_accountId = id; }
	/**
	 * Set object revision to this cache.
	 */
	void SetRev(const MojObject& rev)		{ m_rev = rev; }


	// Getters
	/**
	 * Get MojoDB ID for this cache.
	 */
	const MojObject& GetId() const			{ return m_id; }
	/**
	 * Get account ID from this cache.
	 */
	const MojObject& GetAccountId() const 	{ return m_accountId; }
	/**
	 * Get object revision from this cache.
	 */
	const MojObject& GetRev() const			{ return m_rev; }


	/**
	 * Get the cache object for deleted emails.
	 */
	DeletedEmailsCache&	GetDeletedEmailsCache()	{ return m_deletedEmails; }

	/**
	 * Get the cache object for emails beyond sync window.
	 */
	OldEmailsCache&		GetOldEmailsCache()		{ return m_oldEmails; }

	/**
	 * Returns true if either deleted cache or old emails cache is changed.
	 */
	bool				HasChanged()			{ return m_deletedEmails.HasChanged() || m_oldEmails.HasChanged(); }
private:
	DeletedEmailsCache 	m_deletedEmails;
	OldEmailsCache		m_oldEmails;
	MojObject			m_id;
	MojObject			m_accountId;
	MojObject			m_rev;
};

#endif /* UIDCACHE_H_ */
