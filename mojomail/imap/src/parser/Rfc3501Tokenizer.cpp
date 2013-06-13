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

/*
 * @file	Rfc3901Tokenizer.cpp
 */

#include <string>
#include <algorithm>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "parser/Rfc3501Tokenizer.h"
#include "parser/Util.h"

using namespace std;

const string Rfc3501Tokenizer::KEYWORD_BODYSTRUCTURE	= "BODYSTRUCTURE";
const string Rfc3501Tokenizer::KEYWORD_ENVELOPE 		= "ENVELOPE";
const string Rfc3501Tokenizer::KEYWORD_EXISTS	 		= "EXISTS";
const string Rfc3501Tokenizer::KEYWORD_FLAGS			= "FLAGS";
const string Rfc3501Tokenizer::KEYWORD_INTERNALDATE		= "INTERNALDATE";
const string Rfc3501Tokenizer::KEYWORD_LIST				= "LIST";
const string Rfc3501Tokenizer::KEYWORD_UIDVALIDITY 		= "UIDVALIDITY";
const string Rfc3501Tokenizer::KEYWORD_UID 				= "UID";
const string Rfc3501Tokenizer::NIL_STRING 				= "NIL";
const string Rfc3501Tokenizer::NAMESPACE_STRING 		= "NAMESPACE";


TokenType Rfc3501Tokenizer::throwError(const char* msg) {
	m_error = msg;
	return TK_ERROR;
}

string Rfc3501Tokenizer::valueUpper() {
	return boost::to_upper_copy(token);
}


bool Rfc3501Tokenizer::numberValue(long int& result) {
	return Util::from_string(result, token, std::dec);
}

bool Rfc3501Tokenizer::match(const string& strchr) {
	if (tokenType != TK_TEXT)
		return false;
	
	return boost::iequals(token, strchr);
}

TokenType Rfc3501Tokenizer::next() {
	token.clear();

	tokenType = TK_ERROR;
	if (cp>=len) {
		return tokenType;
	}
	pos = cp;
	char c = chars[cp++];
	switch (c) {
		case '\r':
			token.push_back(c);
			tokenType = TK_CR;
			return tokenType;
		case '\n':
			token.push_back(c);
			tokenType = TK_LF;
			return tokenType;
		case '(':
			token.push_back(c);
			tokenType = TK_LPAREN;
			return tokenType;
		case ')':
			token.push_back(c);
			tokenType = TK_RPAREN;
			return tokenType;
		case '[':
			token.push_back(c);
			tokenType = TK_LBRACKET;
			return tokenType;
		case ']':
			token.push_back(c);
			tokenType = TK_RBRACKET;
			return tokenType;
		case '}':
			token.push_back(c);
			tokenType = TK_RBRACE;
			return tokenType;
		case '%':
			token.push_back(c);
			tokenType = TK_TEXT;
			return tokenType;
		case '*':
			token.push_back(c);
			tokenType = TK_TEXT;
			return tokenType;
		case '\\':
			token.push_back(c);
			tokenType = TK_BACKSLASH;
			return tokenType;
		case ' ': {
			token.push_back(c);
			tokenType = TK_SP;
			return tokenType;
		}

		case '{': {
			if (brace_is_token) {
				token.push_back(c);
				tokenType = TK_LBRACE;
				return tokenType;
			}
			int size = 0;
			c = chars[cp++];

			// FIXME this could overflow size
			while (c>='0'&&c<='9') {
				size = size*10+(c-'0');
				c = chars[cp++];
			}
			if (c != '}')
				return throwError("right brace expected");
			if (len < cp+2)
				return throwError("two chars after brace expected");
			c = chars[cp++];
			if (c != '\r')
				return throwError("cr after brace expected");
			c = chars[cp++];
			if (c != '\n')
				return throwError("crlf after brace expected");
			token.clear();

			if (cp + size <= len) {
				//fprintf(stderr, "got enough bytes: %d < %d", cp+size, len);
				for (int i=0;i<size;i++)
					token.push_back(chars[cp++]);
				tokenType = TK_QUOTED_STRING;
			} else {
				//fprintf(stderr, "need more bytes: %d\n", size);
				bytesNeeded = size;
				return throwError("missing literal data");
			}
			return tokenType;
		}
		case '\"': {
			if (cp>=len) {
				tokenType = TK_QUOTED_STRING_WITHOUT_TERMINATION;
				return tokenType;
 			}
			token.clear();
			c = chars[cp++];
			for (;;) {
				if (c=='\"') {
					tokenType = TK_QUOTED_STRING;
					return tokenType;
				}
				if (c=='\\') {
					if (cp>=len) {
						tokenType = TK_QUOTED_STRING_WITHOUT_TERMINATION;
						return tokenType;
					}
					c = chars[cp++];
				}
				if (cp>=len) {
					tokenType = TK_QUOTED_STRING_WITHOUT_TERMINATION;
					return tokenType;
				}
				token.push_back(c);
				c = chars[cp++];
			}
		}

		default: {
			// Assume this is an atom
			token.push_back(c);
			if (cp<len) {
				c = chars[cp++];
				for (;;) {
					if (isAtomSpecial(c)) {
						cp--;
						tokenType = TK_TEXT;
						return tokenType;
					}
					token.push_back(c);
					if (cp>=len)
						break;
					c = chars[cp++];
				}
			}
			tokenType = TK_TEXT;
			return tokenType;
		}
	}
}

bool Rfc3501Tokenizer::isAtomSpecial(char c) {
	if (c<' ')
		return true;
	switch(c) {
	case '(':
	case ')':
	case '[':
	case ']':
	case '{':
	case '}':
	case '%':
	case '*':
	case '\\':
	case '"':
	case ' ':
		return true;
	default:
		return false;
	}
}


bool Rfc3501Tokenizer::startAt(string key) {
	for (;;) {
		char tokenType = next();
		if (tokenType==0)
			break;
		if (tokenType=='I' && match(key)) {
			return true;
		}
	}
	return false;
}

// Produce a sanitized (no strings) version of the text for debugging.
// This will only work if the buffer is actually tokenizable (no improperly strings, etc)
std::string Rfc3501Tokenizer::produceDebugText()
{
	std::string debugText;

	while(true) {
		next();

		if (tokenType == TK_ERROR) {
			break;
		}

		bool isString = (tokenType == TK_QUOTED_STRING) || (tokenType == TK_LITERAL_BYTES)
				|| tokenType == (TK_QUOTED_STRING_WITHOUT_TERMINATION);

		if (isString) {
			debugText.append(!token.empty() ? "\"[STRING]\"" : "\"\"");
		} else {
			debugText.append(token);
		}
	}

	return debugText;
}
