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

#include "stream/LineReader.h"
#include "CommonPrivate.h"
#include "stream/BaseOutputStream.h"
#include <glib.h>

using namespace std;

LineReader::LineReader(const InputStreamPtr& inputStream)
: InputStreamSink(inputStream),
  m_eof(false),
  m_expect(EXPECT_NONE),
  m_sizeExpected(0),
  m_lineLengthExceeded(false),
  m_timeoutExceeded(false),
  m_maximumLineLength(1048576), // 1MB
  m_timeoutId(0),
  m_newlineMode(NewlineMode_CRLF),
  m_dataAvailableSignal(this)
{
}

LineReader::~LineReader()
{
	if(m_expect != EXPECT_NONE)
		fprintf(stderr, "linereader deleted while waiting for line");

	if(m_timeoutId > 0) {
		g_source_remove(m_timeoutId);
	}
}

bool LineReader::Waiting()
{
	return m_expect != EXPECT_NONE;
}

void LineReader::WaitForLine(LineAvailableSignal::SlotRef slot, int timeout)
{
	if(m_expect != EXPECT_NONE) {
		throw MailException("WaitForLine called while LineReader busy", __FILE__, __LINE__);
	}
	
	if(m_eof) {
		throw MailNetworkDisconnectionException("unexpected end of stream", __FILE__, __LINE__);
	}
	
	m_expect = EXPECT_LINE;
	m_dataAvailableSignal.connect(slot);
	
	SetTimeout(timeout);

	m_lineLengthExceeded = false;
	m_timeoutExceeded = false;
	
	if(m_source.get())
		m_source->StartReading();
	else
		throw MailException("LineReader not connected to input source", __FILE__, __LINE__);
}

void LineReader::WaitForData(DataAvailableSignal::SlotRef slot, size_t expected, int timeout)
{
	if(m_expect != EXPECT_NONE) {
		throw MailException("WaitForData called while LineReader busy", __FILE__, __LINE__);
	}
	
	if(m_eof) {
		throw MailNetworkDisconnectionException("unexpected end of stream", __FILE__, __LINE__);
	}
	
	m_expect = EXPECT_DATA;
	m_sizeExpected = expected;
	m_dataAvailableSignal.connect(slot);
	
	SetTimeout(timeout);

	if(m_source.get())
		m_source->StartReading();
	else
		throw MailException("LineReader not connected to input source", __FILE__, __LINE__);
}

void LineReader::SetTimeout(int timeout)
{
	// Remove old timeout
	if(m_timeoutId > 0) {
		g_source_remove(m_timeoutId);
		m_timeoutId = 0;
	}
	
	if(timeout > 0) {
		m_timeoutId = g_timeout_add_seconds(timeout, LineReader::HandleTimeout, gpointer(this));
	}
}

void LineReader::SetNewlineMode(NewlineMode mode)
{
	m_newlineMode = mode;
}

void LineReader::CheckError()
{
	if( unlikely(m_timeoutExceeded) ) {
		throw MailNetworkTimeoutException("timeout exceeded reading line", __FILE__, __LINE__);
	}
	
	if( unlikely(m_lineLengthExceeded) ) {
		throw MailException("line length exceeded reading line", __FILE__, __LINE__);
	}
}

string LineReader::ReadLine(bool includeNewline)
{
	CheckError();
	
	if( unlikely(m_currentBufferPos == NULL) ) {
		throw MailException("LineReader::ReadLine called without WaitForLine", __FILE__, __LINE__);
	}

	if(!MoreLinesInBuffer()) {
		throw MailNetworkDisconnectionException("no line available", __FILE__, __LINE__);
	}
	
	string s(m_currentBufferPos, includeNewline ? m_currentLineEnd : m_currentNewlinePos);
	
	// adjust buffer pointer; skips over line and \r\n
	m_currentBufferPos = m_currentLineEnd;
	m_currentLineEnd = NULL;
	
	return s;
}

size_t LineReader::NumBytesAvailable()
{
	if( unlikely(m_currentBufferPos == NULL) ) {
		return 0;
	}
	
	return m_currentBufferEnd - m_currentBufferPos;
}

size_t LineReader::ReadData(char* dst, size_t length)
{
	CheckError();
	
	if( unlikely(m_currentBufferPos == NULL) ) {
		throw MailException("LineReader::ReadData called without WaitForLine", __FILE__, __LINE__);
	}
	
	size_t bufferLength = m_currentBufferEnd - m_currentBufferPos;
	size_t bytesToCopy = bufferLength < length ? bufferLength : length;
	
	memcpy(dst, m_currentBufferPos, bytesToCopy);
	m_currentBufferPos += bytesToCopy;
	
	return bytesToCopy;
}

size_t LineReader::WriteFromBuffer(const OutputStreamPtr& os, size_t length)
{
	CheckError();

	if( unlikely(m_currentBufferPos == NULL) ) {
		throw MailException("LineReader::WriteFromBuffer called without WaitForLine", __FILE__, __LINE__);
	}

	size_t bufferLength = m_currentBufferEnd - m_currentBufferPos;
	size_t bytesToCopy = bufferLength < length ? bufferLength : length;

	os->Write(m_currentBufferPos, length);
	m_currentBufferPos += length;

	return bytesToCopy;
}

bool LineReader::IsEOF()
{
	return m_eof;
}

bool LineReader::MoreLinesInBuffer()
{
	if( unlikely(m_currentBufferPos == NULL) ) {
		return false;
	}
	
	if(m_currentLineEnd == NULL || m_currentLineEnd <= m_currentBufferPos) {
		m_currentLineEnd = FindEndOfLine(m_newlineMode, m_currentBufferPos, m_currentBufferEnd, m_currentNewlinePos);
	}
	
	return m_currentLineEnd != NULL;
}

const char* LineReader::FindEndOfLine(NewlineMode newlineMode, const char* start, const char* end, const char*& newlinePos)
{
	if(newlineMode == NewlineMode_CRLF) {
		if(end - start >= 2) {
			// If there's least two characters

			--end; // only loop up to the next-to-last char

			for(const char* pos = start; pos < end; ++pos) {
				if(pos[0] == '\r' && pos[1] == '\n') {
					newlinePos = pos;
					return pos + 2; // skip \r\n
				}
			}
		}
	} else if(newlineMode == NewlineMode_Auto) {
		for(const char* pos = start; pos < end; ++pos) {
			if(pos[0] == '\r') {
				if(end - pos > 0 && pos[1] == '\n') {
					newlinePos = pos;
					return pos + 2; // \r\n
				} else {
					newlinePos = pos;
					return pos + 1; // \n\r
				}
			} else if(pos[0] == '\n') {
				newlinePos = pos;
				return pos + 1;
			}
		}
	}
	
	newlinePos = NULL;
	return NULL;
}

void LineReader::SetupBufferPointers(const char* data, size_t length)
{
	m_currentBufferPos = data;
	m_currentBufferEnd = data + length; // points one past end of buffer
	m_currentLineEnd = NULL;
	m_currentNewlinePos = NULL;
}

size_t LineReader::HandleData(const char* data, size_t length, bool eof)
{
	if(eof) {
		m_eof = true;
	}

	SetupBufferPointers(data, length);

	if(m_expect == EXPECT_LINE) {
		// Find \r\n sequence
		const char* end = FindEndOfLine(m_newlineMode, data, data + length, m_currentNewlinePos);
		
		if (m_maximumLineLength > 0 && ((end != NULL && (size_t)(end - data) > m_maximumLineLength) ||
										(end == NULL && length > m_maximumLineLength)) ) {

			if (m_timeoutId != 0) {
				g_source_remove(m_timeoutId);
				m_timeoutId = 0;
			}

			m_expect = EXPECT_NONE; // must be set before callback
			m_lineLengthExceeded = true;
			m_dataAvailableSignal.fire();
			
		} else if(end != NULL) {
			m_currentLineEnd = end;

			if (m_timeoutId != 0) {
				g_source_remove(m_timeoutId);
				m_timeoutId = 0;
			}

			m_expect = EXPECT_NONE; // must be set before callback
			m_dataAvailableSignal.fire();
		} else if(eof) {

			if (m_timeoutId != 0) {
				g_source_remove(m_timeoutId);
				m_timeoutId = 0;
			}

			m_expect = EXPECT_NONE; // must be set before callback
			m_dataAvailableSignal.fire();
		}
	} else if(m_expect == EXPECT_DATA) {
		// Check if there's enough data to pass on, or EOF
		if(length >= m_sizeExpected || eof) {
			if (m_timeoutId != 0) {
				g_source_remove(m_timeoutId);
				m_timeoutId = 0;
			}

			m_expect = EXPECT_NONE; // must be set before callback
			m_dataAvailableSignal.fire();
		}
	}
	
	size_t bytesHandled = 0;
	
	if(m_currentBufferPos) {
		bytesHandled = m_currentBufferPos - data;

		// Buffer is no longer valid once we return
		m_currentBufferPos = NULL;

		assert(bytesHandled <= length);
	}
	
	if(m_expect == EXPECT_NONE && m_source.get()) {
		m_source->DoneReading();
	}

	return bytesHandled;
}

gboolean LineReader::HandleTimeout(gpointer data)
{
	MojRefCountedPtr<LineReader> that = (LineReader*)data;
	
	that->m_timeoutId = 0;
	
	that->m_timeoutExceeded = true;

	that->m_expect = EXPECT_NONE; // must be set before callback
	that->m_dataAvailableSignal.fire();

	if(that->m_source.get())
		that->m_source->DoneReading();
	
	return FALSE;
}

void LineReader::Status(MojObject& status)
{
	MojErr err;

	const char* expect = "???";
	switch(m_expect) {
	case EXPECT_NONE:
		expect = "none";
		break;
	case EXPECT_LINE:
		expect = "line";
		break;
	case EXPECT_DATA:
		expect = "data";

		err = status.put("bytesExpected", (MojInt64) m_sizeExpected);
		ErrorToException(err);
		break;
	}

	err = status.putString("expecting", expect);
	ErrorToException(err);

	if(m_eof) {
		err = status.put("eof", m_eof);
		ErrorToException(err);
	}

	if(m_lineLengthExceeded) {
		err = status.put("lineLengthExceeded", m_lineLengthExceeded);
		ErrorToException(err);
	}

	if(m_timeoutExceeded) {
		err = status.put("timeoutExceeded", m_timeoutExceeded);
		ErrorToException(err);
	}
}
