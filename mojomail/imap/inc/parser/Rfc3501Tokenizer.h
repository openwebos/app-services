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

/*
 * @file	Rfc3901Tokenizer.h
 *
 * Copyright 2009 Palm, Inc.  All rights reserved.
 */


#ifndef RFC3501TOKENIZER_H_
#define RFC3501TOKENIZER_H_

#include <string.h>
#include "core/MojRefCount.h"
#include "parser/Parser.h"

class Rfc3501TokenizerException : public std::exception
{
	std::string m_message;
public:
	Rfc3501TokenizerException(std::string msg) : m_message(msg)
	{
	}

	virtual ~Rfc3501TokenizerException() throw() {}
};

/*
 * This is a tokenizer to be used when parsing responses as specified in rfc3501.
 */
class Rfc3501Tokenizer : public MojRefCounted
{

public:
	inline Rfc3501Tokenizer(const std::string& str, bool braces = false)
	: brace_is_token(braces), bytesNeeded(-1)
	{
		reset(str);
	}

	void pushBinaryData() {
		tokenType = TK_LITERAL_BYTES;
		bytesNeeded = -1;
	}

	inline void reset(const std::string& str) {
		pos = 0;
		cp = 0;
		tokenType = TK_ERROR;
		bytesNeeded = -1;
		chars.assign(str);
		len = chars.size();
	}

	inline void append(const std::string& str) {
		cp = pos;
		chars.append(str);
		len = chars.size();
	}

	// Erase everything before the current token
	inline void compact() {
		chars.erase(0, pos); // pos is the start of the current token
		cp = 0;
		pos = 0;
		len = chars.size();
	}

	inline const std::string& value() const {
		return token;
	}

	// Get the text in the buffer
	const std::string& getAllText() const {
		return chars;
	}

	std::string produceDebugText();

	std::string getConsumedText() {
		return chars.substr(0,cp);
	}

	std::string getRemainingText() {
		return chars.substr(cp);
	}

	bool numberValue(long int& result);

	std::string valueUpper();

	bool match(const std::string& strchr);

	TokenType next();

	bool startAt(std::string key);

	// Only valid in brace_is_token = false mode
	bool needsLiteralBytes() { return bytesNeeded >= 0; }
	int getBytesNeeded() { return bytesNeeded; }

	static const std::string KEYWORD_BODYSTRUCTURE;
	static const std::string KEYWORD_ENVELOPE;
	static const std::string KEYWORD_EXISTS;
	static const std::string KEYWORD_FLAGS;
	static const std::string KEYWORD_INTERNALDATE;
	static const std::string KEYWORD_LIST;
	static const std::string KEYWORD_UIDVALIDITY;
	static const std::string KEYWORD_UID;
	static const std::string NIL_STRING;
	static const std::string NAMESPACE_STRING;

public:
	std::string token;
	TokenType tokenType;

protected:
	TokenType throwError(const char* msg);
	static bool isAtomSpecial(char c);

	std::string chars;
	int pos;
	int len;
	int cp;

	std::string m_error;
	bool brace_is_token;
	int bytesNeeded;
};



#endif /* RFC3501TOKENIZER_H_ */
