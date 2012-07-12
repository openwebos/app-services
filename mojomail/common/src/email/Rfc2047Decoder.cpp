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

#include "email/Rfc2047Decoder.h"

// for debug
#include <iostream>
using namespace std;

#include <unicode/utypes.h>
#include <unicode/ucnv.h>
#include <unicode/ustring.h>

#include "glib/gbase64.h"
#include "glib.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/shared_array.hpp>
#include "email/CharsetSupport.h"
#include "exceptions/MailException.h"
#include "util/StringUtils.h"

void Rfc2047Decoder::Decode(const std::string& text, std::string& out)
{
	// check for "=?" at beginning and "?=" at end
	if((text[0] != '=') || (text[1] != '?') || (text[text.length() - 2] != '?') || (text[text.length() - 1] != '=')) {
		throw MailException("invalid RFC 2047", __FILE__, __LINE__);
	}

	unsigned int encodingPos = text.find('?', 2);
	if(encodingPos == string::npos) {
		// '?' not found
		throw MailException("invalid RFC 2047", __FILE__, __LINE__);
	}

	// todo add test cases!
	if(text.length() < (encodingPos + 5) || text[encodingPos + 2] != '?') {
		// another expected '?' not found or string not long enough.
		throw MailException("invalid RFC 2047", __FILE__, __LINE__);
	}

	string charset(text.substr(2, encodingPos - 2));
	charset = CharsetSupport::SelectCharset(charset.c_str());

	bool doConversionToUTF8 = false;
	if(!boost::iequals(charset, "utf-8")) {
		// need to do conversion
		doConversionToUTF8 = true;
	}

	string outTemp;
	string& outRef = outTemp;

	switch(text[encodingPos + 1]) {
	case 'b': case 'B': // base64 encoded
	{
		gsize outSize;
		string b64string(text.substr(encodingPos+3,text.length() - (encodingPos + 5)));

		if (b64string.length() == 0) {
			// g_base64_decode doesn't like it if the string is empty
			return;
		}

		guchar *outStr = g_base64_decode(b64string.c_str(), &outSize);

		if (outStr) {
			outRef.assign((char *)outStr);
			g_free(outStr);
		}

		break;
	}
	case 'q': case 'Q': // Q encoded
		outRef.assign("");
		for(unsigned int i = encodingPos + 3; i < (text.length() - 2); ++i) {
			const char ch = text[i];
			if(ch == '_') {
				outRef.push_back(' ');
			} else if(ch == '=') {
				if(i < (text.length() - 4)) {
					int chInt = 0;
					switch(tolower(text[i+1])) {
					case '0': chInt = 0  << 4; break;
					case '1': chInt = 1  << 4; break;
					case '2': chInt = 2  << 4; break;
					case '3': chInt = 3  << 4; break;
					case '4': chInt = 4  << 4; break;
					case '5': chInt = 5  << 4; break;
					case '6': chInt = 6  << 4; break;
					case '7': chInt = 7  << 4; break;
					case '8': chInt = 8  << 4; break;
					case '9': chInt = 9  << 4; break;
					case 'a': chInt = 10 << 4; break;
					case 'b': chInt = 11 << 4; break;
					case 'c': chInt = 12 << 4; break;
					case 'd': chInt = 13 << 4; break;
					case 'e': chInt = 14 << 4; break;
					case 'f': chInt = 15 << 4; break;
					default: outRef.push_back(ch); continue; // possible error case
					}
					switch(tolower(text[i+2])) {
					case '0': chInt += 0 ; break;
					case '1': chInt += 1 ; break;
					case '2': chInt += 2 ; break;
					case '3': chInt += 3 ; break;
					case '4': chInt += 4 ; break;
					case '5': chInt += 5 ; break;
					case '6': chInt += 6 ; break;
					case '7': chInt += 7 ; break;
					case '8': chInt += 8 ; break;
					case '9': chInt += 9 ; break;
					case 'a': chInt += 10; break;
					case 'b': chInt += 11; break;
					case 'c': chInt += 12; break;
					case 'd': chInt += 13; break;
					case 'e': chInt += 14; break;
					case 'f': chInt += 15; break;
					default: outRef.push_back(ch); continue; // possible error case
					}

					outRef.push_back((char)chInt);
					i += 2;
				} else {
					// perhaps malformed text
					outRef.push_back(ch);
				}
			} else {
				outRef.push_back(ch);
			}

		}
	}

	if(doConversionToUTF8) {

		UErrorCode errorCode(U_ZERO_ERROR);
		UConverter* toUnicodeConverter = NULL;
		UConverter* toUtf8Converter = NULL;

		try {
			toUnicodeConverter = ucnv_open(charset.c_str(), &errorCode);

			if(toUnicodeConverter == NULL) {
				throw MailException(("error opening converter for charset " + charset).c_str(), __FILE__, __LINE__);
			}

			toUtf8Converter = ucnv_open("UTF-8", &errorCode);

			if(toUtf8Converter == NULL) {
				throw MailException("error opening UTF-8 converter", __FILE__, __LINE__);
			}

			int srcMinCharSize = ucnv_getMinCharSize(toUnicodeConverter);
			if(srcMinCharSize < 1)
				srcMinCharSize = 1; // avoid divide-by-zero

			int maxUtfCharSize = ucnv_getMaxCharSize(toUtf8Converter);
			int destLen = UCNV_GET_MAX_BYTES_FOR_STRING(outRef.length() / srcMinCharSize, maxUtfCharSize) + 10;
			// was this: int destLen = outRef.length() * 2 + 99;

			boost::shared_array<char> dest (new char[destLen]); // todo get a better handle on how long this needs to be.
			int finalDestLen = ucnv_convert("UTF-8", charset.c_str(),
					dest.get(), destLen - 1,
					outRef.c_str(), outRef.length(),
					&errorCode);

			if(finalDestLen >= destLen || errorCode != U_ZERO_ERROR) {
				throw MailException("conversion to UTF-8 failed", __FILE__, __LINE__);
			}

			out.assign(dest.get());

		} catch(...) {
			if(toUnicodeConverter) {
				ucnv_close(toUnicodeConverter);
				toUnicodeConverter = NULL;
			}

			if(toUtf8Converter) {
				ucnv_close(toUtf8Converter);
				toUtf8Converter = NULL;
			}

			// rethrow exception
			throw;
		}

		if(toUnicodeConverter)
			ucnv_close(toUnicodeConverter);

		if(toUtf8Converter)
			ucnv_close(toUtf8Converter);
	} else {
		// Make sure the text is valid UTF8
		if(!g_utf8_validate(outRef.data(), outRef.size(), NULL)) {
			throw MailException("invalid UTF-8", __FILE__, __LINE__);
		}

		out = outRef;
	}
}

void Rfc2047Decoder::DecodeText(const string& text, string& out)
{
	out.clear();

	size_t lastBlockEnd = 0;

	while(true) {
		size_t blockStart = text.find("=?", lastBlockEnd);
		if(blockStart != string::npos && blockStart < text.size() - 2) {
			size_t encodingStart = text.find('?', blockStart + 2);
			if(encodingStart == string::npos || encodingStart == text.size()) {
				break;
			}

			size_t encodingEnd = text.find('?', encodingStart + 1);
			if(encodingEnd == string::npos || encodingEnd == text.size()) {
				break;
			}

			size_t blockEnd = text.find("?=", encodingEnd + 1);

			if(blockEnd != string::npos) {
				blockEnd += 2; // skip over ?=

				string preceding = text.substr(lastBlockEnd, blockStart - lastBlockEnd);

				// check if there was anything besides whitespace since last block
				if(lastBlockEnd == 0 || preceding.find_first_not_of(" \n\r\t") != string::npos) {
					// append everything since the last block
					StringUtils::SanitizeASCII(preceding);
					out.append(preceding);
				}

				// decode text and append it
				string decoded;
				try {
					Decode(text.substr(blockStart, blockEnd - blockStart), decoded);
					out.append(decoded);
				} catch(const std::exception& e) {
					// Couldn't decode; just append the text without decoding
					string temp = text.substr(blockStart, blockEnd - blockStart);
					StringUtils::SanitizeASCII(temp);
					out.append(temp);
				}

				lastBlockEnd = blockEnd;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	// end of string; append remaining text
	string temp = text.substr(lastBlockEnd);
	StringUtils::SanitizeASCII(temp);
	out.append(temp);
}

string Rfc2047Decoder::SafeDecodeText(const std::string& text)
{
	string out;
	try {
		DecodeText(text, out);
	} catch (...) {
		out = text;
		StringUtils::SanitizeASCII(out);
	}

	return out;
}
