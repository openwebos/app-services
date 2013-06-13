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

#ifndef APPENDRESPONSEPARSER_H_
#define APPENDRESPONSEPARSER_H_

#include "protocol/ImapResponseParser.h"
#include "ImapCoreDefs.h"
#include <vector>

using namespace std;

class ImapCommandResult;

class AppendResponseParser : public ImapResponseParser
{
public:
	AppendResponseParser(ImapSession& session);
	AppendResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot);
	virtual ~AppendResponseParser();

	void HandleResponse(ImapStatusCode status, const string& line);
	bool HandleContinuationResponse();

	void SetContinuationResponseSlot(ContinuationSignal::SlotRef continuationSlot);

	static bool ParseUids(const std::string& line, UID& uid, UID& uidvaildity);

	UID GetUid(){return m_uid;}
	UID GetUidVaildity(){return m_uidValidity;}
protected:
	UID	m_uid;
	UID	m_uidValidity;
};

#endif /* APPENDRESPONSEPARSER_H_ */
