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

#ifndef BUFFEREDRESPONSEPARSER_H_
#define BUFFEREDRESPONSEPARSER_H_

#include "protocol/ImapResponseParser.h"

class TokenBuffer;

class BufferedResponseParser : public ImapResponseParser
{
	static const unsigned int MAX_LITERAL_SIZE = 1024;
	static const unsigned int MAX_LINE_LENGTH = 1024 * 1024; // 1 MB

public:
	BufferedResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot);
	virtual ~BufferedResponseParser();

	bool HandleAdditionalData();

	// Returns true if done parsing
	bool CheckResponseReady();

	// Abstract method called when a line is ready
	virtual void ResponseLineReady() = 0;

	std::string m_buffer;

	size_t	m_bytesNeeded;
	bool	m_needsLiteral;
};

#endif /* BUFFEREDRESPONSEPARSER_H_ */
