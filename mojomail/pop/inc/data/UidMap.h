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

#ifndef UIDMAP_H_
#define UIDMAP_H_

#include "data/OldEmailsCache.h"
#include "data/PopEmail.h"
#include <map>
#include <string>
#include <vector>

/**
 * A map that contains message UID as key and message number as value.
 */
class UidMap
{
public:
	class MessageInfo {
	public:
		MessageInfo(): m_msgNum(0), m_size(0) { }
		MessageInfo(const MessageInfo& info) : m_uid(info.m_uid), m_msgNum(info.m_msgNum), m_size(info.m_size) { }
		~MessageInfo() { }

		void SetUid(const std::string& uid)	{ m_uid = uid; }
		void SetMessageNumber(int msgNum)	{ m_msgNum = msgNum; }
		void SetSize(int size)				{ m_size = size; }

		int	GetMessageNumber()				{ return m_msgNum; }
		int	GetSize()						{ return m_size; }
		const std::string& GetUid() const	{ return m_uid; }
	private:
		std::string 	m_uid;
		int				m_msgNum;
		int 			m_size;
	};
	typedef boost::shared_ptr<MessageInfo>			MessageInfoPtr;
	typedef std::map<std::string, MessageInfoPtr> 	UidInfoMap;

	UidMap();
	~UidMap();

	void				AddToMap(const std::string uid, int num);
	void				SetMessageSize(int num, int size);
	int 				GetMessageNumber(const std::string& uid);
	int 				GetMessageSize(const std::string& uid);
	bool 				HasUid(const std::string& uid);
	int 				Size();
	void				Reset();
	const UidInfoMap& 	GetUidInfoMap() const;
protected:
	typedef std::map<int, MessageInfoPtr>	MsgNumInfoMap;

	bool				HasMessageNumber(int num);

	UidInfoMap			m_uidInfoMap;
	MsgNumInfoMap		m_msgNumInfoMap;
};

#endif /* UIDMAP_H_ */
