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

#include "protocol/NamespaceResponseParser.h"
#include <boost/algorithm/string/predicate.hpp>
#include "parser/Rfc3501Tokenizer.h"
#include "ImapPrivate.h"

NamespaceResponseParser::NamespaceResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
: ImapResponseParser(session, doneSlot)
{
}

NamespaceResponseParser::~NamespaceResponseParser()
{
}

bool NamespaceResponseParser::HandleUntaggedResponse(const string& line)
{
	if(boost::istarts_with(line, "NAMESPACE")) {
		Rfc3501Tokenizer t(line);
		if (t.next() != TK_TEXT || !t.match(Rfc3501Tokenizer::NAMESPACE_STRING))
			ThrowParseException("expected NAMESPACE");
		if (t.next() != TK_SP)
			ThrowParseException("expected sp");

		if (t.next() == TK_LPAREN) {
			// personal namespace

			if (t.next() != TK_LPAREN)
				ThrowParseException("expected lparen");

			// prefix
			if (t.next() == TK_QUOTED_STRING)
				m_namespacePrefix = t.value();
			else if(t.tokenType == TK_NIL || (t.tokenType == TK_TEXT && t.value() == t.NIL_STRING))
				m_namespacePrefix = "";
			else
				ThrowParseException("expected namespace prefix");

			// don't care about hierarchy delimiter or anything else

		} else if(t.tokenType == TK_NIL) {
			// no namespace
		}

		// ignore other namespaces

		return true;
	}

	return false;
}
