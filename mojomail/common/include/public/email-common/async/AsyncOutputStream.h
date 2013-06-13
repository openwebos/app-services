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

#ifndef ASYNCOUTPUTSTREAM_H_
#define ASYNCOUTPUTSTREAM_H_

#include "stream/ByteBufferOutputStream.h"
#include "async/AsyncIOChannel.h"
#include <string>
#include <boost/exception_ptr.hpp>

class AsyncOutputStream : public ByteBufferOutputStream
{
public:
	AsyncOutputStream(AsyncIOChannel* channel);
	virtual ~AsyncOutputStream();
	
	// Overrides ByteBufferOutputStream
	void Write(const char* src, size_t length);

	// Overrides ByteBufferOutputStream
	void Flush(FlushType flushType = FullFlush);
	
	// Overrides ByteBufferOutputStream
	void Close();

protected:
	void FlushBuffer();
	MojErr ChannelWriteable();
	MojErr ChannelClosed();
	
	void Watch();
	void Unwatch();
	
	inline bool ShouldFlushBuffer() const;

	void Error(const std::exception& e);

	static const int BUFFER_SIZE;

	AsyncIOChannel*		m_channel;
	bool				m_watching;
	bool				m_eof;
	bool				m_flush;
	bool				m_closed;

	boost::exception_ptr		m_exception;

	AsyncIOChannel::WriteableSignal::Slot<AsyncOutputStream>	m_writeableSlot;
	AsyncIOChannel::ClosedSignal::Slot<AsyncOutputStream>		m_closedSlot;
};

#endif /*ASYNCOUTPUTSTREAM_H_*/
