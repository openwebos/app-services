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

#include "mimeparser/Rfc822Tokenizer.h"
#include <cctype>

using namespace std;

size_t Rfc822Tokenizer::NextToken(const char* data, size_t length, int flags, Rfc822Token& token)
{
	const char* pos = data;
	const char* end = data + length;

	token.type = Rfc822Token::Token_Error;
	token.value.clear();

	while (true) {
		// Skip over any spaces
		for (; isspace(*pos) && pos < end; pos++) {}

		if (pos >= end) {
			token.type = Rfc822Token::Token_EOF;
			return pos - data;
		}

		char c = *pos;

		if (c == '"') {
			// string
			token.value.clear();

			bool escapeNextChar = false;

			for (pos = pos + 1; pos < end; pos++) {
				char c = *pos;

				if (c == '"' && !escapeNextChar) {
					pos++;
					break;
				} else if (c == '\\' && !escapeNextChar) {
					escapeNextChar = true;
					continue;
				}

				escapeNextChar = false;
				token.value.push_back(c);
			}

			token.type = Rfc822Token::Token_String;
			return pos - data;
		} else if (c == '(') {
			// comment
			bool escapeNextChar = false;
			int nestLevel = 1;
			for (pos = pos + 1; pos < end; pos++) {
				char c = *pos;
				if (c == '(' && !escapeNextChar) {
					nestLevel++;
				} else if (c == ')' && !escapeNextChar) {
					nestLevel--;

					if (nestLevel <= 0) {
						pos++;
						break;
					}
				} else if (c == '\\' && !escapeNextChar) {
					escapeNextChar = true;
				}

				escapeNextChar = false;
			}
			continue; // skip over comment
		} else if (IsSymbol(c, flags & Rfc822TokenizerFlags::Tokenizer_ParameterMode)) {
			token.type = Rfc822Token::Token_Symbol;
			token.value.assign(pos, 1);
			return pos - data + 1;
		} else {
			// atom or other
			const char* begin = pos;

			if (flags & Rfc822TokenizerFlags::Tokenizer_ParameterMode) {
				for (; pos < end; pos++) {
					char c = *pos;
					if (isspace(c) || IsTSpecial(c) || iscntrl(c)) {
						break;
					}
				}
			} else {
				for (; pos < end; pos++) {
					char c = *pos;
					if (isspace(c) || IsSpecial(c) || iscntrl(c)) {
						break;
					}
				}
			}

			token.type = pos > begin ? Rfc822Token::Token_Atom : Rfc822Token::Token_Error;
			token.value.assign(begin, pos - begin);
			return pos - data;
		}
	}
}

/*	specials    =  "(" / ")" / "<" / ">" / "@"  ; Must be in quoted-
 *				/  "," / ";" / ":" / "\" / <">  ;  string, to use
 *				/  "." / "[" / "]"              ;  within a word.
 *
 *  tspecials :=  "(" / ")" / "<" / ">" / "@" /
 *                 "," / ";" / ":" / "\" / <">
 *                 "/" / "[" / "]" / "?" / "="
 */
inline bool Rfc822Tokenizer::IsSpecial(char c)
{
	switch (c) {
	case '(': case ')': case '<': case '>': case '@':
	case ',': case ';': case ':': case '\\': case '"':
	case '.': case '[': case ']':
		return true;
	default:
		return false;
	}
}

inline bool Rfc822Tokenizer::IsTSpecial(char c)
{
	switch (c) {
	case '(': case ')': case '<': case '>': case '@':
	case ',': case ';': case ':': case '\\': case '"':
	case '[': case ']': case '/': case '?': case '=':
		return true;
	default:
		return false;
	}
}

bool Rfc822Tokenizer::IsSymbol(char c, bool parameterMode)
{
	return parameterMode ? IsTSpecial(c) : IsSpecial(c);
}

void Rfc822StringTokenizer::SetLine(const string& line)
{
	m_line = line;
	m_offset = 0;
	m_lastOffset = 0;
}

bool Rfc822StringTokenizer::NextToken(Rfc822Token& token, int flags)
{
	m_lastOffset = m_offset;
	m_offset += m_tokenizer.NextToken(m_line.data() + m_offset, m_line.length() - m_offset, flags, token);
	return token.type != Rfc822Token::Token_EOF;
}

void Rfc822StringTokenizer::Unpeek()
{
	m_offset = m_lastOffset;
}
