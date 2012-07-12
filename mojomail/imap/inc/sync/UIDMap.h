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

#ifndef UIDMAP_H_
#define UIDMAP_H_

#include "ImapCoreDefs.h"
#include <vector>

class UIDMap
{
public:
	// Create a UID map. The initial UID list must be sorted.
	UIDMap(const std::vector<UID>& initialUidList, unsigned int msgCount, unsigned int maxSize = 0);
	virtual ~UIDMap();
	
	// Returns the UID for a given message sequence number
	inline UID GetUID(unsigned int num) {
		if(!WithinRange(num))
			return 0;

		return m_uids[num - m_firstMsgNum];
	}
	
	const std::vector<UID>& GetUIDs() const { return m_uids; }

	void GetMissingMsgNums(std::vector<unsigned int>& msgNums) const;

	void UpdateMsgCount(unsigned int newCount);

	// Update a message num with the UID
	void SetMsgUID(unsigned int msgNum, UID uid);

	// Remove a message number from the map
	UID Remove(unsigned int num);

	// Remove oldest messages when there's too many in the map
	void ShrinkToSize(unsigned int maxSize);

protected:
	inline bool WithinRange(unsigned int num) {
		return (num >= m_firstMsgNum && num - m_firstMsgNum < m_uids.size());
	}

	// List of known UIDs, in order by seq num, starting at m_firstMsgNum
	std::vector<UID>	m_uids;
	
	// Lowest message sequence num stored in the map
	// If all messages are in the map, the first message number is 1
	unsigned int		m_firstMsgNum;
	
	// Total number of messages in the folder
	unsigned int		m_msgCount;
};

#endif /*UIDMAP_H_*/
