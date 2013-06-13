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

#ifndef ASYNCIOCHANNEL_H_
#define ASYNCIOCHANNEL_H_

#include <glib.h>
#include "CommonDefs.h"
#include "core/MojSignal.h"
#include "stream/BaseInputStream.h"
#include "stream/BaseOutputStream.h"
#include "CommonErrors.h"
#include <boost/exception_ptr.hpp>

/**
 * Asynchronous IO channel
 */
class AsyncIOChannel : public MojSignalHandler
{
public:
	typedef MojSignal<> ReadableSignal;
	typedef MojSignal<> WriteableSignal;
	typedef MojSignal<> ClosedSignal;
	
	static MojLogger& s_log;
	
	// Open an IO channel for a file
	static MojRefCountedPtr<AsyncIOChannel> CreateFileIOChannel(const char* filename, const char* mode);
	
	// Registers for callback on read
	virtual void WatchReadable(ReadableSignal::SlotRef slot);
	virtual void UnwatchReadable(ReadableSignal::SlotRef slot);
	
	// Registers for callback when writeable
	virtual void WatchWriteable(WriteableSignal::SlotRef slot);
	virtual void UnwatchWriteable(WriteableSignal::SlotRef slot);
	
	// Registers for callback when channel is no longer accessible
	virtual void WatchClosed(ClosedSignal::SlotRef slot);
	virtual void UnwatchClosed(ClosedSignal::SlotRef slot);

	// Shutdown channel
	virtual void Shutdown() = 0;
	
	/**
	 * Reads bytes from the channel. Returns 0 if no data is currently available.
	 * @param eof is set to true if EOF
	 */
	virtual size_t Read(char* buf, size_t count, bool& eof) = 0;
	
	/**
	 * Writes bytes to channel.
	 * @return number of bytes written into buffer (possibly 0)
	 */
	virtual size_t Write(const char* src, size_t count) = 0;
	
	virtual const InputStreamPtr&		GetInputStream() = 0;
	virtual const OutputStreamPtr&		GetOutputStream() = 0;
	
	bool IsClosed() const { return m_closed; }

	// Get error info
	virtual void SetException(const std::exception& e);
	virtual const boost::exception_ptr& GetException() const;
	virtual MailError::ErrorInfo GetErrorInfo() const;

protected:
	AsyncIOChannel();
	virtual ~AsyncIOChannel();
	
	virtual void SetWatchReadable(bool watch);
	virtual void SetWatchWriteable(bool watch);

	int				m_readers;
	int				m_writers;

	ReadableSignal	m_readableSignal;
	WriteableSignal	m_writeableSignal;
	ClosedSignal	m_closedSignal;
	bool			m_closed;
	
	InputStreamPtr		m_inputStream;
	OutputStreamPtr		m_outputStream;

	boost::exception_ptr	m_exception;
	MailError::ErrorInfo	m_errorInfo;
};

class AsyncIOChannelFactory
{
public:
	AsyncIOChannelFactory();
	virtual ~AsyncIOChannelFactory();
	
	virtual MojRefCountedPtr<AsyncIOChannel> OpenFile(const char* filename, const char* mode) = 0;
};

#endif /*ASYNCIOCHANNEL_H_*/
