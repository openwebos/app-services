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

#include <algorithm>
#include "data/PopEmailAdapter.h"
#include "data/UidMap.h"
#include "PopClient.h"

UidMap::UidMap()
{
}

UidMap::~UidMap()
{

}

void UidMap::AddToMap(std::string uid, int num)
{
	MessageInfoPtr info(new MessageInfo);
	info->SetUid(uid);
	info->SetMessageNumber(num);
	info->SetSize(0);

	m_uidInfoMap[uid] = info;
	m_msgNumInfoMap[num] = info;
}

void UidMap::SetMessageSize(int num, int size)
{
	if (!HasMessageNumber(num)) {
		return;
	}

	MessageInfoPtr info = m_msgNumInfoMap[num];
	info->SetSize(size);
}

int UidMap::GetMessageNumber(const std::string& uid)
{
	if (!HasUid(uid)) {
		return 0;
	}

	MessageInfoPtr info = m_uidInfoMap[uid];
	return info->GetMessageNumber();
}

int UidMap::GetMessageSize(const std::string& uid)
{
	if (!HasUid(uid)) {
		return -1;
	}

	MessageInfoPtr info = m_uidInfoMap[uid];
	return info->GetSize();
}

bool UidMap::HasUid(const std::string& uid)
{
	return m_uidInfoMap.find(uid) != m_uidInfoMap.end();
}

bool UidMap::HasMessageNumber(int num)
{
	return m_msgNumInfoMap.find(num) != m_msgNumInfoMap.end();
}

int UidMap::Size()
{
	return m_uidInfoMap.size();
}

void UidMap::Reset()
{
	m_uidInfoMap.clear();
	m_msgNumInfoMap.clear();
}

const UidMap::UidInfoMap& UidMap::GetUidInfoMap() const
{
	return m_uidInfoMap;
}
