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

#ifndef LINEREADER_H_
#define LINEREADER_H_

#include "stream/BaseInputStream.h"
#include <string>
#include <glib.h>

class BaseOutputStream;

class LineReader: public InputStreamSink
{
public:
	typedef MojSignal<> DataAvailableSignal;
	typedef DataAvailableSignal LineAvailableSignal;

	enum NewlineMode {
		NewlineMode_CRLF,
		NewlineMode_Auto
	};

	LineReader(const InputStreamPtr& inputStream);
	virtual ~LineReader();
	
	/**
	 * Gets one line from the stream. Slot will be called when a line is available,
	 * or on EOF/error. Timeout in seconds will trigger, if above zero, and a full line
	 * has not been received.
	 */
	void WaitForLine(DataAvailableSignal::SlotRef slot, int timeout = 0);
	
	/**
	 * Gets a specified number of bytes. Slot will be called when the expected number
	 * of bytes are available, or on EOF/error. Timeout in seconds will trigger, if above zero,
	 * and a full line has not been received.
	 * 
	 * @param expected number of bytes expected
	 */
	void WaitForData(DataAvailableSignal::SlotRef slot, size_t expected, int timeout = 0);
	
	/**
	 * Reads a line of data from the buffer.
	 * 
	 * Must only be called from within a function called by the slot passed to WaitForLine.
	 */
	std::string ReadLine(bool includeCRLF = false);
	
	/**
	 * Checks whether there's any more lines in the current buffer.
	 * If not, you should call WaitForLine again.
	 * 
	 * Must only be called from within a function called by the slot passed to WaitForLine.
	 */
	bool MoreLinesInBuffer();
	
	/**
	 * Returns the number of bytes available in the current buffer.
	 * 
	 * Must only be called from within a function called by the slot passed to WaitForLine.
	 */
	size_t NumBytesAvailable();
	
	/**
	 * Check if the stream has EOF. Note that there may still be data in the buffer.
	 */
	bool IsEOF();
	
	/**
	 * Reads data from a buffer.
	 * 
	 * Must only be called from within a function called by the slot passed to WaitForLine.
	 */
	size_t ReadData(char* dst, size_t length);

	/**
	 * Write data from buffer to a stream
	 */
	size_t WriteFromBuffer(const MojRefCountedPtr<BaseOutputStream>& os, size_t length);

	/**
	 * Returns whether the LineReader is currently waiting for data.
	 */
	bool Waiting();

	/**
	 * Report status
	 */
	void Status(MojObject& status);

	/**
	 * Throw an exception if there was an error.
	 */
	void CheckError();

	/**
	 * Reset the LineReader timeout. The new timeout will start at the current time.
	 * Must already be expecting line/data.
	 *
	 * @param timeout Seconds
	 */
	void SetTimeout(int timeout);

	/**
	 * Sets the newline mode.
	 */
	void SetNewlineMode(NewlineMode mode);

protected:
	// Implements InputStreamSink::HandleData
	size_t	HandleData(const char* data, size_t length, bool eof);
	static gboolean HandleTimeout(gpointer);
	
	inline void SetupBufferPointers(const char* data, size_t length);
	static const char* FindEndOfLine(NewlineMode newlineMode, const char* start, const char* end, const char*& newlinePos);
	
	enum ExpectType {
		EXPECT_NONE,
		EXPECT_LINE,
		EXPECT_DATA
	};
	
	InputStreamPtr	m_inputStream;
	bool			m_eof;
	
	ExpectType		m_expect;
	size_t			m_sizeExpected;
	
	const char*		m_currentBufferPos;
	const char*		m_currentBufferEnd;
	const char*		m_currentLineEnd; // points one past the end of the line
	const char*		m_currentNewlinePos; // points at the newline

	bool			m_lineLengthExceeded;
	bool			m_timeoutExceeded;
	size_t			m_maximumLineLength;
	guint			m_timeoutId;

	NewlineMode		m_newlineMode;

	DataAvailableSignal		m_dataAvailableSignal;
};

typedef MojRefCountedPtr<LineReader> LineReaderPtr;

#endif /*LINEREADER_H_*/
