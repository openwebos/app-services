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

#include "protocol/UidSearchResponseParser.h"
#include <boost/algorithm/string/predicate.hpp>
#include <sstream>

using namespace std;

UidSearchResponseParser::UidSearchResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot, vector<UID>& uidList)
: ImapResponseParser(session, doneSlot), m_uidList(uidList)
{
}

UidSearchResponseParser::~UidSearchResponseParser()
{
}

// Fast UID list parser
bool UidSearchResponseParser::ParseUids(const std::string& line, std::vector<UID>& uids)
{
	UID uidTemp = 0;
	
	for(string::const_iterator it = line.begin(); it != line.end(); ++it) {
		char c = *it;
		
		if(c == ' ') {
			if(uidTemp > 0) { // uids can't be zero
				uids.push_back(uidTemp);
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
		uids.push_back(uidTemp);
	}
	
	return true;
}

bool UidSearchResponseParser::HandleUntaggedResponse(const string& line)
{
	if(boost::istarts_with(line, "SEARCH")) {
		string word; // should be search
		string rest;
		
		SplitOnce(line, word, rest);
		
		ParseUids(rest, m_uidList);
	}
	return false;
}

void UidSearchResponseParser::HandleResponse(ImapStatusCode status, const string& response)
{
	// TODO handle response
}
