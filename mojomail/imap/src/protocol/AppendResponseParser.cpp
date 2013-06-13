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

#include "protocol/AppendResponseParser.h"
#include "ImapStatus.h"

AppendResponseParser::AppendResponseParser(ImapSession& session)
: ImapResponseParser(session),
  m_uid(0),
  m_uidValidity(0)
{
}

AppendResponseParser::AppendResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
: ImapResponseParser(session, doneSlot),
  m_uid(0),
  m_uidValidity(0)
{
}

AppendResponseParser::~AppendResponseParser()
{
}

void AppendResponseParser::HandleResponse(ImapStatusCode status, const string& line)
{
	if(status == OK)
	{	string rest;
		string tag;
		SplitOnce(line, tag, rest);
		size_t found = rest.find_first_of(']');
		string ids = rest.substr(0, found);		//Get the IDs string.
		ParseUids(ids, m_uid, m_uidValidity);	//Parse the IDs.
	}
}

bool AppendResponseParser::HandleContinuationResponse()
{
	m_continuationSignal.fire();
	return true;
}

void AppendResponseParser::SetContinuationResponseSlot(ContinuationSignal::SlotRef continuationSlot)
{
	m_continuationSignal.connect(continuationSlot);
}

// Fast UID list parser
bool AppendResponseParser::ParseUids(const std::string& line, UID& uid, UID& uidVaildity)
{
	UID uidTemp = 0;

	for(string::const_iterator it = line.begin(); it != line.end(); ++it) {
		char c = *it;

		if(c == ' ') {
			if(uidTemp > 0) { // uids can't be zero
				uidVaildity = uidTemp;
				uidTemp = 0;
			}
		} else if(c >= '0' && c <= '9') {
			UID temp = uidTemp * 10 + ((int)c - '0');

			if(temp >= uidTemp) {
				uidTemp = temp;
			} else {
				// If the number went *down*, we overflowed the unsigned int.
				// UIDs must fit in a 32-bit integer.
				return false;
			}

		} else {
			// Error -- not a number
			return false;
		}
	}

	if(uidTemp > 0) {
		uid=uidTemp;
	}

	return true;
}
