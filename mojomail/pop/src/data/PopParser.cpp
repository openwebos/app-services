// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "data/PopParser.h"
#include <string>

using namespace std;
using std::string;

const char* const PopParser::CRLF 					= "\r\n";
const char* const PopParser::COMMAND_TERMINATION 	= ".\r\n";
const int PopParser::MAX_LINE_LENGTH 				= 16 * 1024; // 16 KB

PopParser::PopParser(Email &message, const string& rd) {
	msg = message;
	in = rd;
	pos = 0;
	isEOF = false;
}

bool PopParser::IsCRLF(char c) {
	if (c == CRLF[0]) {
		c = Peek();
		if (c == CRLF[1]) {
			return true;
		} else {
			return false;
		}
	}
	return false;
}

char PopParser::Peek() {
	return currentSegment[pos + 1];
}

bool PopParser::IsCRLF(const string& str) {

	if (CRLF == str)
		return true;

	string::const_iterator it;
	for (it = str.begin(); it < str.end(); it++ ) {
		if (*it != ' ' && *it != '\t')
			return false;
	}
	// the entire line is white space.  if the last two characters matches CRLF,
	// the current line is considered to be CRLF.
	return CRLF == (str.substr(0, str.length() - 2));
}

bool PopParser::IsEOF() {
	if (lastLine.length() < 2) {
		return false;
	}

	if (COMMAND_TERMINATION == (currentSegment)) {
		isEOF = true;
	}

	return isEOF;
}

bool PopParser::IsFolded(char c) {
	if (c == (char)' ' || c == (char)'\t') {
		return true;
	}
	return false;
}
