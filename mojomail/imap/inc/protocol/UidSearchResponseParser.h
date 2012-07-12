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

#ifndef UIDSEARCHRESPONSEPARSER_H_
#define UIDSEARCHRESPONSEPARSER_H_

#include "ImapCoreDefs.h"
#include "protocol/ImapResponseParser.h"
#include <vector>

class UidSearchResponseParser : public ImapResponseParser
{
public:
	UidSearchResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot, std::vector<UID>& uidList);
	virtual ~UidSearchResponseParser();
	
	bool HandleUntaggedResponse(const std::string& line);
	void HandleResponse(ImapStatusCode status, const std::string& response);
	
	static bool ParseUids(const std::string& line, std::vector<UID>& uids);
	
protected:
	std::vector<UID>&	m_uidList;
};

#endif /*UIDSEARCHRESPONSEPARSER_H_*/
