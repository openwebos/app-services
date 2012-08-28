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


#ifndef RFC822TOKENIZER_H_
#define RFC822TOKENIZER_H_

#include <string>

struct Rfc822Token
{
	enum TokenType
	{
		Token_Error,
		Token_EOF,
		Token_Atom,
		Token_String,
		Token_Symbol
	};

	bool IsSymbol(char c) const { return type == Token_Symbol && value.at(0) == c; }
	char GetSymbol() const { return value.at(0); }
	bool IsAtom() const { return type == Token_Atom; }
	bool IsWord() const { return type == Token_Atom || type == Token_String; }
	bool IsEnd() const { return type == Token_Error || type == Token_EOF; }

	bool IsSymbol(const char* chars)
	{
		if (type != Token_Symbol) return false;

		for ( ; *chars != '\0'; ++chars) {
			if(*chars == value.at(0))
				return true;
		}
		return false;
	}

	TokenType		type;
	std::string			value;
};

namespace Rfc822TokenizerFlags {
	const int Tokenizer_ParameterMode = 1;
}

class Rfc822Tokenizer
{
public:
	size_t NextToken(const char* data, size_t length, int flags, Rfc822Token& token);
	static bool IsSymbol(char c, bool parameterMode);
	static bool IsSpecial(char c);
	static bool IsTSpecial(char c);
};

// Adapter for strings
class Rfc822StringTokenizer
{
public:
	Rfc822StringTokenizer() : m_offset(0), m_lastOffset(0) {}

	// Set the string to parse. Resets any options.
	void SetLine(const std::string& line);

	// Gets the next token (advances position in string)
	bool NextToken(Rfc822Token& token, int flags = 0);

	// Push the last token back (can only be done immediately after call to NextToken)
	void Unpeek();

	std::string		m_line;
	size_t			m_offset;
	size_t			m_lastOffset;
	Rfc822Tokenizer	m_tokenizer;
};

#endif /* RFC822TOKENIZER_H_ */
