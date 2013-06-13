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

#ifndef SEARCHREQUEST_H_
#define SEARCHREQUEST_H_

#include <core/MojRefCount.h>
#include <string>

class MojServiceMessage;

class SearchRequest : public MojRefCounted
{
public:
	SearchRequest();
	virtual ~SearchRequest();

	// Service message which gets responses
	void SetServiceMessage(MojServiceMessage* msg) { m_msg.reset(msg); }
	const MojRefCountedPtr<MojServiceMessage>& GetServiceMessage() const { return m_msg; }

	// Set search text, which matches text in headers or body
	void SetSearchText(const std::string& searchText) { m_searchText = searchText; }
	const std::string& GetSearchText() const { return m_searchText; }

	// Max results to return
	void SetLimit(int limit) { m_limit = limit; }
	int GetLimit() const { return m_limit; }

protected:

	MojRefCountedPtr<MojServiceMessage>	m_msg;
	std::string		m_searchText;
	int				m_limit;
};

#endif /* SEARCHREQUEST_H_ */
