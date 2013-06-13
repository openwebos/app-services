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

#include "protocol/ExamineResponseParser.h"
#include "parser/Rfc3501Tokenizer.h"
#include "parser/Util.h"

using namespace std;

ExamineResponseParser::ExamineResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
: ImapResponseParser(session, doneSlot),
  m_exists(-1), m_uidValidity(0)
{
}

ExamineResponseParser::~ExamineResponseParser()
{
}


bool ExamineResponseParser::HandleUntaggedResponse(const string& line)
{
	Rfc3501Tokenizer t(line);

	if(t.next() != TK_TEXT)
		return false;

	if(t.value() == "FLAGS") {
		// TODO
		return true;
	} else if(t.value() == "OK") {
		if(t.next() != TK_SP)
			return false;
		if(t.next() != TK_LBRACKET)
			return false;
		if(t.next() != TK_TEXT)
			return false;

		if(t.match("UIDVALIDITY")) {
			if(t.next() != TK_SP)
				ThrowParseException("expected space after UIDVALIDITY");

			MojUInt64 uidValidity = 0; // 64-bit to deal with buggy servers (usa.net)
			if(t.next() != TK_TEXT || !Util::from_string(uidValidity, t.value(), std::dec))
				ThrowParseException("expected number");

			if (uidValidity > MojUInt32Max) {
				MojLogWarning(m_log, "server folder UIDVALIDITY exceeds 32-bit limit");
			}

			m_uidValidity = (UID) uidValidity; // force cast to 32-bit (hack)

			return true;
		}
	} else {
		long number;
		if(t.numberValue(number)) {
			if(t.next() != TK_SP)
				return false;

			if(t.next() == TK_TEXT && t.valueUpper() == "EXISTS") {
				m_exists = number;
				return true;
			}
		}
	}

	// Let someone else handle this response
	return false;
}

