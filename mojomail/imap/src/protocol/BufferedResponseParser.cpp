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

#include "protocol/BufferedResponseParser.h"
#include "parser/Rfc3501Tokenizer.h"
#include "client/ImapSession.h"
#include "parser/Util.h"

using namespace std;

BufferedResponseParser::BufferedResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
: ImapResponseParser(session, doneSlot),
  m_bytesNeeded(0), m_needsLiteral(false)
{
}

BufferedResponseParser::~BufferedResponseParser()
{
}

bool BufferedResponseParser::CheckResponseReady()
{
	bool needsLiteral = false;
	size_t bytesNeeded = 0;

	if (m_buffer.size() >= 3 && m_buffer[m_buffer.size() - 1] == '}') {
		size_t numStart = m_buffer.rfind('{');

		if (numStart != string::npos) {
			string numString = m_buffer.substr(numStart + 1, m_buffer.size() - 1);

			if(!numString.empty() && Util::from_string(bytesNeeded, numString, std::dec)) {
				needsLiteral = true;
			}
		}
	}

	if (needsLiteral) {
		// Check if the size is sane
		if (bytesNeeded > MAX_LITERAL_SIZE || m_buffer.size() + bytesNeeded > MAX_LINE_LENGTH) {
			throw MailException("literal size exceeds maximum", __FILE__, __LINE__);
		}

		MojRefCountedPtr<ImapResponseParser> ref(this);

		m_bytesNeeded = bytesNeeded;
		m_needsLiteral = true;

		MojLogDebug(m_log, "requesting %d bytes for continuation", (int) m_bytesNeeded);

		m_session.RequestData(ref, bytesNeeded);

		return false; // needs more data
	} else {
		m_needsLiteral = false;

		ResponseLineReady();

		m_buffer.clear();

		return true; // done
	}
}

bool BufferedResponseParser::HandleAdditionalData()
{
	MojRefCountedPtr<LineReader>& lineReader = m_session.GetLineReader();

	if (m_needsLiteral) {
		// Read raw bytes
		size_t bytesAvailable = lineReader->NumBytesAvailable();

		char buf[MAX_LITERAL_SIZE + 10];

		if ( (size_t) m_bytesNeeded > bytesAvailable || m_bytesNeeded > MAX_LITERAL_SIZE) {
			MojLogError(m_log, "need %d but only %d available", (size_t) m_bytesNeeded, bytesAvailable);
			throw MailException("refusing to parse literal", __FILE__, __LINE__);
		}

		// FIXME copy string directly
		lineReader->ReadData(buf, m_bytesNeeded);

		// Append newline to buffer
		m_buffer.append("\r\n");

		// Add byte data to buffer
		m_buffer.append(buf, m_bytesNeeded);

		// Done getting this literal
		m_needsLiteral = false;

		// Read the rest of the line
		MojRefCountedPtr<ImapResponseParser> ref(this);
		m_session.RequestLine(ref);

		return true;
	} else {
		// Reading the rest of the line
		string line = lineReader->ReadLine();
		m_buffer.append(line);

		return !CheckResponseReady();
	}
}
