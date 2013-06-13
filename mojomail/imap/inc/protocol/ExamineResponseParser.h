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

#ifndef EXAMINERESPONSEPARSER_H_
#define EXAMINERESPONSEPARSER_H_

#include "protocol/ImapResponseParser.h"

class ExamineResponseParser : public ImapResponseParser
{
public:
	ExamineResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot);
	virtual ~ExamineResponseParser();
	
	bool HandleUntaggedResponse(const std::string& line);

	// Get the number of messages in the inbox, or -1 if unknown
	int		GetExistsCount() const { return m_exists; }

	// Get the UIDVALIDITY for the folder
	UID		GetUIDValidity() const { return m_uidValidity; }

protected:
	int		m_exists;
	UID		m_uidValidity;
};

#endif /*EXAMINERESPONSEPARSER_H_*/
