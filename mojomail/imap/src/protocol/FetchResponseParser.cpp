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

#include "protocol/FetchResponseParser.h"
#include <boost/regex.hpp>
#include "parser/ImapParser.h"
#include "parser/Rfc3501Tokenizer.h"
#include "parser/SemanticActions.h"
#include "data/ImapEmail.h"
#include "client/ImapSession.h"
#include <algorithm>
#include "stream/ByteBufferOutputStream.h"
#include "stream/CounterOutputStream.h"

using namespace std;

const size_t READ_BUFFER_SIZE = 8 * 1024; // 8K
const size_t MAX_INCOMPLETE_STRING_SIZE = 1 * 1024 * 1024; // 1MB
const int READ_TIMEOUT = 90; // 90 seconds to read 8K = 0.1 KB/second

FetchResponseParser::FetchResponseParser(ImapSession& session)
: ImapResponseParser(session),
  m_literalBytesRemaining(0),
  m_recoveringFromError(false)
{
}

FetchResponseParser::FetchResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
: ImapResponseParser(session, doneSlot),
  m_literalBytesRemaining(0),
  m_recoveringFromError(false)
{
}

FetchResponseParser::~FetchResponseParser()
{
}

void FetchResponseParser::SetPartOutputStream(const OutputStreamPtr& outputStream)
{
	m_partOutputStream = outputStream;
}

bool FetchResponseParser::HandleUntaggedResponse(const string& line)
{
	boost::regex fetchRe("[0-9]+ FETCH", boost::regex::icase);
	if(boost::regex_search(line, fetchRe, boost:: match_continuous)) {
		m_tokenizer.reset(new Rfc3501Tokenizer(line, true));
		
		// Add newline
		m_tokenizer->append("\r\n");
		
		m_semantic.reset(new SemanticActions(*m_tokenizer.get()));
		m_imapParser.reset(new ImapParser(m_tokenizer.get(), m_semantic.get()));
		
		if(m_partOutputStream.get()) {
			m_semantic->SetBodyOutputStream(m_partOutputStream);
		}

		try {
			Parse();
		} catch(const std::exception& e) {
			MojLogError(m_log, "exception parsing fetch response: %s", e.what());
			throw;
		}
		return true;
	}

	return false;
}

bool FetchResponseParser::HandleAdditionalData()
{
	const LineReaderPtr& lineReader = m_session.GetLineReader();
	bool needsMoreData = true;

	// Logic for skipping over unparseable responses
	// While we're in this mode, we'll do the minimum to get to the end of the response
	// FIXME this logic needs to be merged with the parser to reduce code duplication
	// See the normal flow to understand how this is supposed to work
	if (m_recoveringFromError) {
		MojLogDebug(m_log, "read some data in recoveringFromError mode; bytes expected: %d", m_literalBytesRemaining);

		if (m_literalBytesRemaining > 0) {
			size_t bytesAvailable = lineReader->NumBytesAvailable();
			size_t bytesToCopy = min(bytesAvailable, m_literalBytesRemaining);

			boost::scoped_array<char> buf(new char[bytesToCopy]);

			lineReader->ReadData(buf.get(), bytesToCopy);

			m_tokenizer->append(string(buf.get(), bytesToCopy));

			m_literalBytesRemaining -= bytesToCopy;

			MojRefCountedPtr<FetchResponseParser> ref(this);
			// Now we need to get the line after the literal data
			if (m_literalBytesRemaining == 0) {
				MojLogDebug(m_log, "got enough literal bytes to skip; reading another line");
				m_session.RequestLine(ref, READ_TIMEOUT);
			} else {
				MojLogDebug(m_log, "not enough literal bytes to skip; reading the rest");
				m_session.RequestData(ref, m_literalBytesRemaining, READ_TIMEOUT);
			}

			return true;
		} else {
			// Add line to buffer
			string line = lineReader->ReadLine(true);

			// Clear tokenizer contents and append new data
			m_tokenizer->reset( line );

			// advance past error token
			m_tokenizer->next();

			return SkipRestOfResponse();
		}
	}

	if(m_semantic->ExpectingBinaryData()) {
		size_t bytesAvailable = lineReader->NumBytesAvailable();
		size_t bytesToCopy = min(bytesAvailable, m_literalBytesRemaining);

		MojLogDebug(m_log, "reading additional binary data for parser; %d bytes available, %d needed",
				bytesAvailable, m_literalBytesRemaining);

		OutputStreamPtr os = m_semantic->GetCurrentOutputStream();

		if(os.get() == NULL) {
			MojLogWarning(m_log, "%s: no output stream", __PRETTY_FUNCTION__);
			os.reset(new CounterOutputStream());
		}

		// Write to stream
		if(m_literalBytesRemaining > 0) {
			MojLogDebug(m_log, "writing %d bytes to output stream %p", bytesToCopy, os.get());
			size_t bytesWritten = lineReader->WriteFromBuffer(os, bytesToCopy);

			assert( bytesWritten <= m_literalBytesRemaining );
			m_literalBytesRemaining -= bytesWritten;
		}

		if(m_literalBytesRemaining == 0) {
			MojLogDebug(m_log, "parser received all literal data");
			m_tokenizer->reset("");
			m_tokenizer->pushBinaryData();

			// Call back into parser
			needsMoreData = Parse();
		} else {
			RequestMoreData();
		}
	} else {
		MojLogDebug(m_log, "reading additional line for parser");

		if(lineReader->MoreLinesInBuffer()) {
			string line = lineReader->ReadLine(true);
			MojLogDebug(ImapRequestManager::s_protocolLog, "continued: %s", line.c_str());

			// Workaround for Gmail bug where \r\n might appear in the middle of a string (illegal)
			bool incompleteString = (m_tokenizer->tokenType == TK_QUOTED_STRING_WITHOUT_TERMINATION);

			if(incompleteString) {
				// Compact tokenizer contents to save memory
				m_tokenizer->compact();

				// Check for obvious abuse (trying to get around line reader max line length)
				if(m_tokenizer->token.size() > MAX_INCOMPLETE_STRING_SIZE) {
					throw Rfc3501ParseException("excessively long string", __FILE__, __LINE__);
				}

				// Append rest of line to string
				m_tokenizer->append(line);

				// This re-reads the token
				m_tokenizer->next();
			} else {
				// Clear tokenizer contents and append new data
				m_tokenizer->reset( line );

				// advance past error token
				m_tokenizer->next();
			}

			// Call back into parser
			needsMoreData = Parse();
		} else {
			RequestMoreData();
		}
	}

	// Return whether we need more data
	return needsMoreData;
}

void FetchResponseParser::RequestMoreData()
{
	MojRefCountedPtr<FetchResponseParser> ref(this);

	if(m_semantic->ExpectingBinaryData()) {
		// Read a buffer-full of data
		size_t bytesRequested = min<size_t>(m_literalBytesRemaining, READ_BUFFER_SIZE);

		MojLogDebug(m_log, "requesting another %d bytes of binary data", bytesRequested);
		m_session.RequestData(ref, bytesRequested, READ_TIMEOUT);
	} else {
		m_session.RequestLine(ref, READ_TIMEOUT);
	}
}

bool FetchResponseParser::Parse()
{
	MojLogDebug(m_log, "running parser");

	ImapParser::ParseResult result;

	try {
		result = m_imapParser->Parse();
	} catch (const Rfc3501ParseException& e) {
		// Try to skip over the rest of the response
		m_session.SetSafeMode(true);
		return SkipRestOfResponse();
	} catch (const std::exception& e) {
		m_session.SetSafeMode(true);
		throw;
	}

	if(result == ImapParser::PARSE_NEEDS_DATA) {
		//MojLogDebug(m_log, "parser needs more data");

		if(m_semantic->ExpectingBinaryData()) {
			m_literalBytesRemaining = m_semantic->ExpectedDataLength();
		}

		RequestMoreData();
	} else {
		// done parsing
		unsigned int msgNum = m_semantic->GetMsgNum();
		boost::shared_ptr<ImapEmail> email = m_semantic->GetEmail();
		assert( email.get() );
		MojLogInfo(m_log, "parsed email uid=%d msg=%d", email->GetUID(), msgNum);

		m_emails.push_back( FetchUpdate(msgNum, email, m_semantic->GetFlagsUpdated()) );
	}
	
	// Return true if we need more data
	return result == ImapParser::PARSE_NEEDS_DATA;
}

bool FetchResponseParser::SkipRestOfResponse()
{
	MojRefCountedPtr<FetchResponseParser> ref(this);

	// Copy into new tokenizer
	Rfc3501Tokenizer recoveryTokenizer(m_tokenizer->getAllText(), false);
	m_tokenizer->reset("");

	// Extract debug text for logging
	try {
		string sanitizedLine = recoveryTokenizer.produceDebugText();
		MojLogError(m_log, "bad fetch response line (sanitized): %s", sanitizedLine.c_str());
	} catch (std::exception& e) {
		// oops
		MojLogError(m_log, "error parsing line for debug output: %s", e.what());
	}

	if (recoveryTokenizer.tokenType == TK_QUOTED_STRING_WITHOUT_TERMINATION) {
		// Sheesh, this response is just plain borked
		// Better not to try to parse it to avoid security issues
		throw MailException("gave up trying to skip over unparseable response", __FILE__, __LINE__);
	} else if (recoveryTokenizer.needsLiteralBytes()) {
		// Need some more data
		m_literalBytesRemaining = recoveryTokenizer.getBytesNeeded();

		MojLogDebug(m_log, "need %d more bytes to skip unparseable line", m_literalBytesRemaining);
		m_session.RequestData(ref, m_literalBytesRemaining, READ_TIMEOUT);

		m_recoveringFromError = true;
		return true;
	} else {
		// End of the response; just return and move on
		MojLogInfo(m_log, "done skipping unparseable lines");

		m_recoveringFromError = false;
		return false;
	}
}
