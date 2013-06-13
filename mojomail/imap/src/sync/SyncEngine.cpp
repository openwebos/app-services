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

#include "sync/SyncEngine.h"
#include "ImapPrivate.h"
#include <utility>

SyncEngine::SyncEngine(const UIDList& sortedRemote, const UIDList& sortedDeleted,
		const UIDList& sortedUnseen, const UIDList& sortedAnswered,
		const UIDList& sortedFlagged)
: m_remoteUIDs(sortedRemote),
  m_deletedUIDs(sortedDeleted),
  m_unseenUIDs(sortedUnseen),
  m_answeredUIDs(sortedAnswered),
  m_flaggedUIDs(sortedFlagged),
  m_remoteIt(sortedRemote.begin())
{
}

SyncEngine::~SyncEngine()
{
}

/**
 * Compare a local list of UIDs with a remote list of UIDs and determine which entries are
 * missing from either list. Both lists must be in ascending order by UID.
 */
void SyncEngine::Diff(const vector<EmailStub> &localBatch, bool hasMoreLocal)
{
	vector<EmailStub>::const_iterator localIt = localBatch.begin();

	while(localIt != localBatch.end() && m_remoteIt != m_remoteUIDs.end()) {
		UID localUID = localIt->uid;
		UID remoteUID = *m_remoteIt;

		if(localUID == remoteUID) {
			// In both the local and remote lists

			// Check deleted
			if(m_deletedUIDs.Find(localUID)) {
				m_deletedIds.push_back(localIt->id);
			} else {
				// Check flags
				EmailFlags serverFlags;
				serverFlags.read = (m_unseenUIDs.Find(localUID) == false);
				serverFlags.replied = m_answeredUIDs.Find(localUID);
				serverFlags.flagged = m_flaggedUIDs.Find(localUID);

				if(serverFlags != localIt->lastSyncFlags) {
					m_modifiedFlags.push_back( make_pair(*localIt, serverFlags) );
				}
			}

			++m_remoteIt;
			++localIt;
		} else if(remoteUID < localUID) {
			// Only in the remote list, and not deleted
			if(!m_deletedUIDs.Find(remoteUID)) {
				m_newUIDs.push_back(remoteUID);
			}

			++m_remoteIt;
		} else if(remoteUID > localUID) {
			// Only in the local list
			m_deletedIds.push_back(localIt->id);
			++localIt;
		} else {
			// This should never happen
			assert(0);
		}
	}

	// No more remote UIDs; any further local UIDs are not present on the server
	while(localIt != localBatch.end()) {
		m_deletedIds.push_back(localIt->id);
		++localIt;
	}

	if(!hasMoreLocal) {
		// Any remaining remote UIDs are considered new
		while(m_remoteIt != m_remoteUIDs.end()) {
			// Only if not deleted
			if(!m_deletedUIDs.Find(*m_remoteIt)) {
				m_newUIDs.push_back(*m_remoteIt);
			}
			++m_remoteIt;
		}
	}
}

SyncEngine::UIDSet::UIDSet(const UIDList& uidList)
: m_uidList(uidList), m_it(uidList.begin())
{
}

bool SyncEngine::UIDSet::Find(UID uid)
{
	// Skip over any lower UIDs
	while(m_it != m_uidList.end() && *m_it < uid) {
		++m_it;
	}

	if(m_it != m_uidList.end() && *m_it == uid)
		return true;

	return false;
}
