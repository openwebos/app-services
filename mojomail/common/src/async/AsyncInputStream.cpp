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

#include "async/AsyncInputStream.h"
#include "exceptions/MailException.h"

using namespace std;

const int AsyncInputStream::DEFAULT_BUFFER_SIZE = 8192;

AsyncInputStream::AsyncInputStream(AsyncIOChannel* channel)
: m_channel(channel),
  m_wantData(false),
  m_watching(false),
  m_callbackId(0),
  m_eof(false),
  m_readableSlot(this, &AsyncInputStream::ChannelReadable),
  m_closedSlot(this, &AsyncInputStream::ChannelClosed)
{
	m_channel->WatchClosed(m_closedSlot);
}

AsyncInputStream::~AsyncInputStream()
{
	// Remove callback (if any) so it doesn't get called after we've been deleted
	if(m_callbackId > 0) {
		//MojLogDebug(AsyncIOChannel::s_log, "removing flush callback %d from %p", m_callbackId, this);
		g_source_remove(m_callbackId);
	}
}

size_t AsyncInputStream::HandleData(const char* data, size_t length, bool eof)
{
	if(m_sink) {
		// Make sure the sink doesn't get deleted until after returning
		MojRefCountedPtr<InputStreamSink> temp(m_sink);

		if(m_buffer.empty()) {
			size_t bytesHandled = m_sink->HandleData(data, length, eof);
			assert( bytesHandled <= length );

			// Store any unhandled data in the buffer
			m_buffer.append(data + bytesHandled, length - bytesHandled);
		} else {
			// Combine the buffer and the new content
			m_buffer.append(data, length);

			// Call the sink's handleData on the updated buffer
			size_t bytesHandled = m_sink->HandleData(m_buffer.data(), m_buffer.length(), eof);
			assert( bytesHandled <= m_buffer.length() );

			// Delete any handled data from the buffer.
			if(bytesHandled > 0)
				m_buffer.erase(0, bytesHandled);
		}
	} else {
		// If there's no sink, copy everything to the buffer
		m_buffer.append(data, length);
	}

	return 0;
}

void AsyncInputStream::StartReading()
{
	//MojLogDebug(AsyncIOChannel::s_log, "async input stream requested data %p", this);

	// Schedule a callback to flush the buffer when the queue is idle.
	// This is to avoid reentrant calls to handleData.
	
	// Note that we can't check if the buffer is empty yet, because this could
	// be called from within handleData.

	m_wantData = true;

	ScheduleFlush();
}

void AsyncInputStream::ScheduleFlush()
{
	if(m_callbackId == 0) {
		m_callbackId = g_idle_add(&AsyncInputStream::FlushBufferCallback, gpointer(this));
		//MojLogDebug(AsyncIOChannel::s_log, "adding flush callback %d for %p", m_callbackId, this);
	} else {
		//MojLogDebug(AsyncIOChannel::s_log, "flush callback %d pending already %p", m_callbackId, this);
	}
}

gboolean AsyncInputStream::FlushBufferCallback(gpointer data)
{
	AsyncInputStream* stream = reinterpret_cast<AsyncInputStream*>(data);
	assert(stream);

	// Hold temp ref so it doesn't get deleted
	MojRefCountedPtr<AsyncInputStream> temp(stream);

	stream->FlushBuffer();
	
	return false;
}

void AsyncInputStream::FlushBuffer()
{
	// Clear m_callbackId
	m_callbackId = 0;

	//MojLogDebug(AsyncIOChannel::s_log, "flushing buffer for input stream %p", this);

	try {
		if(!m_buffer.empty() || (m_wantData && m_eof)) {
			// HandleData will prepend the buffer to this empty string
			HandleData("", 0, m_eof);
		}

		if(m_wantData) {
			Watch();
		}
	} catch(const std::exception& e) {
		// FIXME handle error
		MojLogError(AsyncIOChannel::s_log, "exception in %s: %s", __PRETTY_FUNCTION__, e.what());
		Error(e);
	} catch(...) {
		MojLogError(AsyncIOChannel::s_log, "exception in %s", __PRETTY_FUNCTION__);
	}
}

void AsyncInputStream::DoneReading()
{
	MojLogDebug(AsyncIOChannel::s_log, "async input stream %p done reading", this);

	// Stop watching
	m_wantData = false;
	Unwatch();
}

void AsyncInputStream::Watch()
{
	if(!m_watching && !m_eof) {
		if(m_channel)
			m_channel->WatchReadable(m_readableSlot);
		m_watching = true;
	}
}

void AsyncInputStream::Unwatch()
{
	if(m_watching) {
		if(m_channel)
			m_channel->UnwatchReadable(m_readableSlot);
		m_watching = false;
	}
}

void AsyncInputStream::ReadFromChannel()
{
	if(m_eof || !m_wantData) {
		Unwatch();
		return;
	}

	char buf[DEFAULT_BUFFER_SIZE];
	
	int bytesToRead = DEFAULT_BUFFER_SIZE;
	bool eof = false;
	size_t bytesRead;
	
	do {
		if(m_channel == NULL)
			throw MailNetworkDisconnectionException("connection closed", __FILE__, __LINE__);

		bytesRead = m_channel->Read(buf, bytesToRead, eof);
		
		if(bytesRead > 0) {
			HandleData(buf, bytesRead, false);
		} else if(eof) {
			Unwatch();
			m_eof = true;
			m_wantData = false;
			
			// Flush buffer
			HandleData("", 0, true);
		}
	} while(bytesRead > 0 && m_wantData);
	
	return;
}

MojErr AsyncInputStream::ChannelReadable()
{
	try {
		ReadFromChannel();
	} catch(const std::exception& e) {
		// FIXME propagate error
		MojLogError(AsyncIOChannel::s_log, "exception in %s: %s", __PRETTY_FUNCTION__, e.what());
		Error(e);
	} catch(...) {
		// FIXME propagate error
		MojLogError(AsyncIOChannel::s_log, "unknown exception in %s", __PRETTY_FUNCTION__);
	}
	return MojErrNone;
}

MojErr AsyncInputStream::ChannelClosed()
{
	MojLogInfo(AsyncIOChannel::s_log, "AsyncInputStream closed");

	m_channel = NULL;
	m_eof = true;

	ScheduleFlush();

	return MojErrNone;
}

void AsyncInputStream::Error(const exception& e)
{
	Unwatch();

	if(!m_eof) {
		m_eof = true;

		ScheduleFlush();
	}
}
