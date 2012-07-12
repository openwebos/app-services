// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef POPPARSER_H_
#define POPPARSER_H_

#include <string>
#include "data/Email.h"

class PopParser {
public:
	PopParser(Email &msg, const std::string& rd);

	/* constants */
	static const char* const CRLF;
	static const char* const COMMAND_TERMINATION;
	static const int MAX_LINE_LENGTH;


	// Lets not do nextline. just accept a line and parse it.
	// have a class that listens for new data from the server. When it sees
	// a newline, writes string to buffer and allows this class to parse
	void NextLine(); // throws IOException, MessageParseException
	void ReadLine(); // throws IOException, MessageParseException
	char NextChar();
	char Peek();
	char GetPeekChar();
	bool IsCRLF(char c);
	bool IsCRLF(const std::string& str);
	bool IsEOF();
	bool IsFolded(char c);

private:
	Email msg;
	std::string in;
	int pos;
	bool isEOF;
	std::string lastLine;
	std::string currentSegment;
};

#endif /* POPPARSER_H_*/
