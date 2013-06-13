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

#include "sync/UIDMap.h"
#include <algorithm>

using namespace std;

UIDMap::UIDMap(const vector<UID>& initialUidList, unsigned int msgCount, unsigned int maxSize)
{
	// Copy the uids (must already be sorted)
	unsigned int listSize = maxSize > 0 ? std::min(maxSize, initialUidList.size()) : initialUidList.size();

	m_uids.insert(m_uids.begin(), initialUidList.end() - listSize, initialUidList.end());

	// Make sure there's no duplicates
	std::unique(m_uids.begin(), m_uids.end());

	// If we get back more results than expected, increase the message count
	// It might just indicate that we haven't gotten an EXISTS response yet
	// FIXME make sure this is safe / latest count
	if(m_uids.size() > msgCount) {
		msgCount = m_uids.size();
	}

	m_msgCount = msgCount;

	// Calculate the first message number.
	// If we have the entire folder in the map, then the first message should be number 1.
	m_firstMsgNum = msgCount + 1 - m_uids.size();

	//printf("msgCount = %d, initial = %d, first = %d\n", msgCount, initialUidList.size(), m_firstMsgNum);
}

UIDMap::~UIDMap()
{
}

void UIDMap::UpdateMsgCount(unsigned int newCount)
{
	m_msgCount = newCount;

	// Resize the UID list. Message numbers with an unknown UID will be zero'd out.
	m_uids.resize(m_msgCount + 1 - m_firstMsgNum);
}

void UIDMap::GetMissingMsgNums(std::vector<unsigned int>& msgNums) const
{
	vector<UID>::const_iterator it;

	for(unsigned int i = 0; i < m_uids.size(); i++) {
		if(m_uids[i] == 0) {
			msgNums.push_back(m_firstMsgNum + i);
		}
	}
}

void UIDMap::SetMsgUID(unsigned int msgNum, UID uid)
{
	if(WithinRange(msgNum)) {
		m_uids[msgNum - m_firstMsgNum] = uid;
	}
}

UID UIDMap::Remove(unsigned int num)
{
	UID uid = 0;

	if(WithinRange(num)) {
		// If the message is within our map range, delete it
		size_t offset = num - m_firstMsgNum;

		uid = m_uids[offset];
		m_uids.erase(m_uids.begin() + offset);
	}

	if(num <= m_firstMsgNum) {
		// If the deleted message is before our map range, or the first message
		--m_firstMsgNum;
	}

	--m_msgCount;

	return uid;
}

