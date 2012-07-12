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

#include "async/AsyncOutputStream.h"
#include "exceptions/MailException.h"
#include "exceptions/ExceptionUtils.h"

using namespace std;

const int AsyncOutputStream::BUFFER_SIZE = 16 * 1024; // 16KB

AsyncOutputStream::AsyncOutputStream(AsyncIOChannel* channel)
: ByteBufferOutputStream(BUFFER_SIZE),
  m_channel(channel),
  m_watching(false),
  m_eof(false),
  m_flush(false),
  m_closed(false),
  m_writeableSlot(this, &AsyncOutputStream::ChannelWriteable),
  m_closedSlot(this, &AsyncOutputStream::ChannelClosed)
{
	m_channel->WatchClosed(m_closedSlot);
}

AsyncOutputStream::~AsyncOutputStream()
{
}

void AsyncOutputStream::Write(const char* src, size_t length)
{
	if(m_exception != NULL) {
		boost::rethrow_exception(m_exception);
	}

	if(m_closed || m_channel == NULL) {
		throw MailNetworkDisconnectionException("can't write to closed channel", __FILE__, __LINE__);
	}
	
	ByteBufferOutputStream::Write(src, length);
	//fprintf(stderr, "adding %d bytes to buffer, %d total\n", length, m_buffer.length());

	// When the buffer is full, start writing out data
	if(IsFull()) {
		Watch();
	}

	// FIXME: Currently we'll flush every time to preserve backwards compatibility.
	// This should be removed once everyone does explicit flushes when needed.
	Flush();
}

void AsyncOutputStream::Flush(FlushType flushType)
{
	// Since this is the final stream, partial flushes will be ignored
	if(flushType != PartialFlush) {
		m_flush = true;

		// The flush will occur when the writeable callback gets called
		Watch();
	}
}


void AsyncOutputStream::Watch()
{
	if(!m_watching) {
		if(m_channel == NULL) {
			throw MailException("can't watch; channel closed", __FILE__, __LINE__);
		}

		m_channel->WatchWriteable(m_writeableSlot);
		m_watching = true;
	}
}

void AsyncOutputStream::Unwatch()
{
	if(m_watching) {
		if(m_channel)
			m_channel->UnwatchWriteable(m_writeableSlot);
		m_watching = false;
	}
}

bool AsyncOutputStream::ShouldFlushBuffer() const
{
	return m_buffer.length() >= m_softSizeLimit || m_flush;
}

void AsyncOutputStream::FlushBuffer()
{
	//fprintf(stderr, "FlushBuffer: %d bytes in buffer, m_flush = %d\n", m_buffer.size(), m_flush);

	while(!m_buffer.empty() && ShouldFlushBuffer()) {
		if(m_channel == NULL) {
			throw MailException("can't write; channel closed", __FILE__, __LINE__);
		}

		size_t bytesWritten = m_channel->Write(m_buffer.data(), m_buffer.length());
		//fprintf(stderr, "wrote %d/%d bytes to channel\n", bytesWritten, m_buffer.length());

		if(bytesWritten > 0) {
			// Delete written bytes from buffer
			m_buffer.erase(0, bytesWritten);
		} else {
			// Couldn't write anything from the buffer
			break;
		}
	}

	if(m_buffer.length() < m_softSizeLimit) {
		// Indicate that there's space in the buffer to write
		m_writeableSignal.call();
	}

	if(!m_buffer.empty()) {
		// Decide if we still want to watch right now
		if(ShouldFlushBuffer())
			Watch();
		else
			Unwatch();
	} else {
		// Buffer is empty
		m_flush = false; // done flushing all the data

		Unwatch();

		if(m_eof && m_channel != NULL) {
			m_channel->Shutdown();
		}
	}

	return;
}

MojErr AsyncOutputStream::ChannelWriteable()
{
	try {
		FlushBuffer();
	} catch(const std::exception& e) {
		MojLogError(AsyncIOChannel::s_log, "exception in %s: %s", __PRETTY_FUNCTION__, e.what());
		Error(e);
	} catch(...) {
		MojLogError(AsyncIOChannel::s_log, "exception in %s", __PRETTY_FUNCTION__);
	}
	
	return MojErrNone;
}

MojErr AsyncOutputStream::ChannelClosed()
{
	MojLogDebug(AsyncIOChannel::s_log, "%s: channel closed", __PRETTY_FUNCTION__);

	m_closed = true;
	m_channel = NULL;

	return MojErrNone;
}

void AsyncOutputStream::Close()
{
	m_flush = true;
	m_eof = true;

	if(m_buffer.empty()) {
		if(m_channel != NULL)
			m_channel->Shutdown();
	} else {
		Watch();
	}
}

void AsyncOutputStream::Error(const exception& e)
{
	Unwatch();

	m_exception = ExceptionUtils::CopyException(e);
	m_writeableSignal.fire();
}
