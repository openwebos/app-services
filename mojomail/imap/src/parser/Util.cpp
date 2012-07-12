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
 * @file	Util.cpp
 *
 * Copyright 2009 Palm, Inc.  All rights reserved.
 */

#include <string>
#include <sstream>
#include <iostream>
#include <iomanip>
#include "parser/Util.h"
#include <time.h>
#include "exceptions/Rfc3501ParseException.h"
#include "email/DateUtils.h"

using namespace std;

static char STANDARD_ALPHABET[] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T', 'U',
    'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u',
    'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '+', ','
};

static char UTF7_DECODABET[] =
    {
        -9,-9,-9,-9,-9,-9,-9,-9,-9,                 // Decimal  0 -  8
        -5,-5,                                      // Whitespace: Tab and Linefeed
        -9,-9,                                      // Decimal 11 - 12
        -5,                                         // Whitespace: Carriage Return
        -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,     // Decimal 14 - 26
        -9,-9,-9,-9,-9,                             // Decimal 27 - 31
        -5,                                         // Whitespace: Space
        -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,              // Decimal 33 - 42
        62,                                         // Plus sign at decimal 43
        63,											// Comma at decimal 44
        -9,-9,-9,                                   // Decimal 45 - 47
        52,53,54,55,56,57,58,59,60,61,              // Numbers zero through nine
        -9,-9,-9,                                   // Decimal 58 - 60
        -1,                                         // Equals sign at decimal 61
        -9,-9,-9,                                      // Decimal 62 - 64
        0,1,2,3,4,5,6,7,8,9,10,11,12,13,            // Letters 'A' through 'N'
        14,15,16,17,18,19,20,21,22,23,24,25,        // Letters 'O' through 'Z'
        -9,-9,-9,-9,-9,-9,                          // Decimal 91 - 96
        26,27,28,29,30,31,32,33,34,35,36,37,38,     // Letters 'a' through 'm'
        39,40,41,42,43,44,45,46,47,48,49,50,51,     // Letters 'n' through 'z'
    };

static bool utf8_to_wstring(wstring& dest, const string& src) {
	bool err = false;
	dest.clear();
	wchar_t w = 0;
	int bytes = 0;
	const wchar_t errChar = L'�';
	for (size_t i = 0; i < src.size(); i++) {
		unsigned char c = (unsigned char) src[i];
		if (c <= 0x7f) {//first byte
			if (bytes) {
				dest.push_back(errChar);
				bytes = 0;
				err = true;
			}
			dest.push_back((wchar_t) c);
		} else if (c <= 0xbf) {//second/third/etc byte
			if (bytes) {
				w = ((w << 6) | (c & 0x3f));
				bytes--;
				if (bytes == 0)
					dest.push_back(w);
			} else {
				dest.push_back(errChar);
				err = true;
			}
		} else if (c <= 0xdf) {
			//2byte sequence start
			bytes = 1;
			w = c & 0x1f;
		} else if (c <= 0xef) {//3byte sequence start
			bytes = 2;
			w = c & 0x0f;
		} else if (c <= 0xf7) {//3byte sequence start
			bytes = 3;
			w = c & 0x07;
		} else {
			dest.push_back(errChar);
			bytes = 0;
			err = true;
		}
	}
	if (bytes) {
		dest.push_back(errChar);
		err = true;
	}
	return err;
}

static bool wstring_to_utf8(string& dest, const wstring& src) {
	bool err = false;
	dest.clear();
	for (size_t i = 0; i < src.size(); i++) {
		wchar_t w = src[i];
		if (w <= 0x7f)
			dest.push_back((char) w);
		else if (w <= 0x7ff) {
			dest.push_back(0xc0 | ((w >> 6) & 0x1f));
			dest.push_back(0x80 | (w & 0x3f));
		} else if (w <= 0xffff) {
			dest.push_back(0xe0 | ((w >> 12) & 0x0f));
			dest.push_back(0x80 | ((w >> 6) & 0x3f));
			dest.push_back(0x80 | (w & 0x3f));
		} else if (w <= 0x10ffff) {
			dest.push_back(0xf0 | ((w >> 18) & 0x07));
			dest.push_back(0x80 | ((w >> 12) & 0x3f));
			dest.push_back(0x80 | ((w >> 6) & 0x3f));
			dest.push_back(0x80 | (w & 0x3f));
		} else {
			dest.push_back('?');
			err = true;
		}
	}
	return err;
}

static void encodeBase64(wstring& result, const wstring& s, int pos, int len) {
		int val = 0;
		int bits = 0;
		for (int i=0;i<len;i++) {
			int c = s[pos++];
			val = (val<<16) | (c&0xff);
			bits += 16;

			while (bits>=6) {
				bits -= 6;
				result.push_back(STANDARD_ALPHABET[val>>bits]);
				int mask_left = ((1<<bits)-1);
				val &= mask_left;
			}
		}
		if (bits>0) {
			val = val << (6-bits);
			result.push_back(STANDARD_ALPHABET[val]);
			val = 0;
		}
	}

	/*
	 * See RFC3501 for specification
	 *
	 * This is not particularly efficient, but it's only used for folder names
	 */

	// Encoded: "My Bogus &AOEA6QDtAPMA+g-"
	// Decoded: "My Bogus áéíóú"

wstring Util::encodeModifiedUtf7(const wstring& s)  {
    	wstring result = L"";
    	int len = s.size();
    	int pos = 0;
    	while (pos<len) {
    		wchar_t c = s[pos];
    		if (c=='&') {
    			pos++;
    			result.push_back(c);
    			result.push_back('-');
    			continue;
    		}
    		if (c>=0) {
    			pos++;
    			result.push_back(c);
    			continue;
    		}
    		int pos2 = pos;
    		while (pos2<len && s[pos2]<0)
    			pos2++;
			result.push_back('&');
    		encodeBase64(result, s, pos, pos2-pos);
    		result.push_back('-');
    		pos = pos2+1;
    	}
    	return result;
    }


string Util::encodeFolderName(const string& folderName) {
	wstring wfolderName;
	string result;
	utf8_to_wstring(wfolderName, folderName);
	wstring quoted = quoteString(encodeModifiedUtf7(wfolderName));
	wstring_to_utf8(result, quoted);
	return result;
}

wstring Util::quoteString(const wstring& txt) {
	wstring result;
	result.push_back('"');
	for (size_t i=0;i<txt.size();i++) {
		wchar_t c = txt[i];
		if (c=='"' || c=='\\')
			result.push_back('\\');
		result.push_back(c);
	}
	result.push_back('"');
	return result;
}


	/*
	 * See RFC3501 for specification
	 *
	 * This is not particularly efficient, but it's only used for folder names
	 */

    /**
     * Translates a Base64 value to either its 6-bit reconstruction value
     * or a negative number indicating some other meaning.
     **/

class DecoderError : public std::exception {
	std::string m_message;
public:
	DecoderError(std::string msg) : m_message(msg) {

	}
	virtual ~DecoderError() throw() {

	}
};

static void decodeBase64(wstring& result, const wstring& s, int pos, int len) {
		int maxl = sizeof(UTF7_DECODABET);
		int val = 0;
		int bits = 0;
		for (int i=0;i<len;i++) {
			unsigned char c = s[pos++];
			if (c>=maxl)
				throw new DecoderError(string("Char not in UTF7 DECODABET: ")+Util::toHexString(c));
			int b = UTF7_DECODABET[c];
			if (b<0) {
				if (b==-5)
					continue;
				throw new DecoderError(string("Char not in UTF7 DECODABET: ")+Util::toHexString(c));
			}
			val = (val<<6) | b;
			bits += 6;
			if (bits>=16) {
				bits -= 16;
				int mask_left = ((1<<bits)-1);

				// FIXME: If this is a unicode character, we need different handling here.
				result.push_back(val>>bits);
				val &= mask_left;
			}
		}
	}


string Util::decodeModifiedUtf7(const string& s) {
		// Throws DecoderError
		//
		wstring ws;
		utf8_to_wstring(ws, s);
    	wstring wresult;
    	string result;
    	size_t len = s.size();
    	size_t pos = 0;
    	while (pos<len) {
    		wchar_t c = ws[pos++];
    		if (c != '&') {
    			wresult.push_back(c);
    			continue;
    		}

    		// find shift out
    		size_t pos2 = pos;
    		while (pos2<len && ws[pos2]!='-')
    			pos2++;
    		if (pos2==pos) {
				pos = pos2+1;
				wresult.push_back('&');
				continue;
    		}
    		decodeBase64(wresult, ws, pos, pos2-pos);
    		pos = pos2+1;
    	}
    	wstring_to_utf8(result, wresult);
    	return result;
    }

string Util::toHexString(int n, int width) {
	std::stringstream out;
	out << "0x"<< setw(width) << setfill('0') << hex << n;
	return out.str();
}

string Util::toString(int n) {
	std::stringstream out;
	out << n;
	return out.str();
}

// Format: "16-Apr-2010 10:32:38 -0400"
MojInt64 Util::ParseInternalDate(const string& s)
{
	time_t time = DateUtils::ParseRfc822Date(s.c_str());
	if(time == -1) {
		throw Rfc3501ParseException("error parsing date", __FILE__, __LINE__);
	}

	return MojInt64(time) * 1000L;
}
