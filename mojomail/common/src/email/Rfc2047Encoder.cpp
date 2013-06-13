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

#include "email/Rfc2047Encoder.h"
#include <cctype>

Rfc2047Encoder::Rfc2047Encoder()
{
}

Rfc2047Encoder::~Rfc2047Encoder()
{
}

// TODO handle wrapping
/*
 * RFC 2047 section 5 regarding encoded-words in address display names:
 *
 * In this case the set of characters that may be used in a "Q"-encoded
 * 'encoded-word' is restricted to: <upper and lower case ASCII
 * letters, decimal digits, "!", "*", "+", "-", "/", "=", and "_"
 * (underscore, ASCII 95.)>.
 */
void Rfc2047Encoder::QEncode(const std::string& text, std::string& out)
{
	const char HEX_DIGITS[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
	
	out.append("=?UTF-8?Q?");
	
	for(std::string::const_iterator it = text.begin(); it != text.end(); ++it) {
		const char c = *it;
		
		if(isalnum(c)) {
			// Letters and digits are encoded as themselves
			out.push_back(c);
		} else {
			// Non-printable characters are hex-encoded
			switch(c) {
			case ' ':
				// 0x20 (space) is encoded as '_'
				out.push_back('_');
				break;
			case '!': case '*': case '+': case '-': case '/':
				out.push_back(c);
				break;
			default:
				// Hex encoding for special and non-printable characters
				out.push_back('=');
				out.push_back(HEX_DIGITS[((unsigned char) c) / 16]);
				out.push_back(HEX_DIGITS[((unsigned char) c) % 16]);
			}
		}
	}
	
	out.append("?=");
}
