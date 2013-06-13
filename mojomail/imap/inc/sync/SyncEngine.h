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

#ifndef SYNCENGINE_H_
#define SYNCENGINE_H_

#include "ImapCoreDefs.h"
#include <vector>
#include "core/MojObject.h"
#include "data/ImapEmail.h"

class SyncEngine {
	typedef std::vector<UID> UIDList;

public:
	SyncEngine(const UIDList& sortedRemote, const UIDList& sortedDeleted,
			const UIDList& sortedUnseen, const UIDList& sortedAnswered,
			const UIDList& sortedFlagged);
	virtual ~SyncEngine();

	// Key info we need to sync in the email
	struct EmailStub
	{
		EmailStub(UID uid, const MojObject& id) : uid(uid), id(id) {}

		UID uid;
		MojObject id;

		EmailFlags lastSyncFlags;
	};

	class UIDSet
	{
	public:
		UIDSet(const UIDList& uidList);
		inline bool Find(UID uid);

	protected:
		const UIDList& m_uidList;
		UIDList::const_iterator m_it;
	};

	/**
	 * Compares a batch of local email stubs with the remote UID lists.
	 *
	 * New email UIDs will be available using GetNewUIDs()
	 * Deleted email ids will be available using GetDeletedIds()
	 */
	void Diff(const std::vector<EmailStub> &localBatch, bool hasMoreLocal);

	/**
	 * Clear the current list of new and deleted ids
	 */
	void Clear() { m_newUIDs.clear(); m_deletedIds.clear(); }

	const std::vector<UID>&			GetNewUIDs() const		{ return m_newUIDs; }
	const std::vector<MojObject>&	GetDeletedIds() const	{ return m_deletedIds; }
	const std::vector< std::pair<EmailStub, EmailFlags> >&	GetModifiedFlags() const { return m_modifiedFlags; }

protected:
	// These must all be sorted
	const UIDList&	m_remoteUIDs;
	UIDSet			m_deletedUIDs;
	UIDSet			m_unseenUIDs;
	UIDSet			m_answeredUIDs;
	UIDSet			m_flaggedUIDs;

	UIDList::const_iterator m_remoteIt;

	std::vector<UID>		m_newUIDs;
	std::vector<MojObject>	m_deletedIds;
	std::vector< std::pair<EmailStub, EmailFlags> >	m_modifiedFlags;
};

#endif /* SYNCENGINE_H_ */
