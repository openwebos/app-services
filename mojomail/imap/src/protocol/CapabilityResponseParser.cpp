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

#include "protocol/CapabilityResponseParser.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "client/ImapSession.h"

CapabilityResponseParser::CapabilityResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
: ImapResponseParser(session, doneSlot)
{
}

CapabilityResponseParser::~CapabilityResponseParser()
{
}

void CapabilityResponseParser::ParseCapabilities(const std::string& line)
{
	boost::char_separator<char> sep(" \t");
	boost::tokenizer< boost::char_separator<char> > tok(line, sep);

	m_session.GetCapabilities().Clear();

	for(boost::tokenizer< boost::char_separator<char> >::iterator it = tok.begin(); it != tok.end(); ++it) {
		if(it == tok.begin() && boost::iequals(*it, "CAPABILITY")) {
			// Skip first token -- should be "CAPABILITY"
		} else {
			m_session.GetCapabilities().SetCapability(*it);
		}
	}

	m_session.GetCapabilities().SetValid(true);
}

bool CapabilityResponseParser::HandleUntaggedResponse(const std::string& line)
{
	if(boost::istarts_with(line, "CAPABILITY")) {
		ParseCapabilities(line);
		return true;
	}
	
	return false;
}

void CapabilityResponseParser::HandleResponse(ImapStatusCode status, const std::string& line)
{
	if(boost::istarts_with(line, "[CAPABILITY")) {
		size_t end = line.find(']');
		ParseCapabilities(line.substr(1, end - 1));
	}
}
