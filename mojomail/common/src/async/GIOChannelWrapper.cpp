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

#include "async/GIOChannelWrapper.h"
#include <boost/shared_ptr.hpp>
#include <glib.h>
#include "exceptions/MailException.h"
#include "exceptions/GErrorException.h"
#include "CommonPrivate.h"
#include "async/AsyncInputStream.h"
#include "async/AsyncOutputStream.h"

using namespace std;

GIOChannelWrapper::GIOChannelWrapper(GIOChannel* channel)
: m_channel(channel),
  m_readers(0),
  m_writers(0),
  m_watchReadId(0),
  m_watchWriteId(0),
  m_closed(false)
{
}

GIOChannelWrapper::~GIOChannelWrapper()
{
	try {
		Shutdown();
	} catch(...) {
		MojLogError(s_log, "exception shutting down at %s:%d", __FILE__, __LINE__);
	}
	
	if(m_channel) {
		g_io_channel_unref(m_channel);
	}
}

MojRefCountedPtr<AsyncIOChannel> GIOChannelWrapper::CreateFileIOChannel(const char* filename, const char* mode)
{
	GError* gerr = NULL;
	GIOChannel* channel = g_io_channel_new_file(filename, mode, &gerr);
	GErrorToException(gerr);
	
	MojRefCountedPtr<GIOChannelWrapper> aioChannel( new GIOChannelWrapper(channel) );
	
	g_io_channel_set_flags(aioChannel->m_channel, G_IO_FLAG_NONBLOCK, &gerr);
	GErrorToException(gerr);
	
	g_io_channel_set_encoding(aioChannel->m_channel, NULL, &gerr);
	GErrorToException(gerr);
	
	return aioChannel;
}

size_t GIOChannelWrapper::Read(char* dst, size_t count, bool& eof)
{
	GError* gerr = NULL; // must be initialized
	gsize bytesRead = 0;
	
	if(m_closed) {
		throw MailNetworkDisconnectionException("channel closed", __FILE__, __LINE__);
	}

	GIOStatus status = g_io_channel_read_chars(m_channel, dst, count, &bytesRead, &gerr);

	switch(status) {
	case G_IO_STATUS_NORMAL:
		return bytesRead;
	case G_IO_STATUS_EOF:
		eof = true;
		ErrorOrEOF();
		return 0;
	case G_IO_STATUS_AGAIN:
		return 0;
	case G_IO_STATUS_ERROR:
		if(gerr) {
			GErrorException exc(gerr, __FILE__, __LINE__);
			g_error_free(gerr);
			SetException(exc);

			ErrorOrEOF();
			throw exc;
		} else {
			// shouldn't ever get here
			throw MailException("unknown read error", __FILE__, __LINE__);
		}
	}
	
	return bytesRead;
}

size_t GIOChannelWrapper::Write(const char* src, size_t count)
{
	if(m_closed) {
		throw MailNetworkDisconnectionException("attempted to write to closed channel", __FILE__, __LINE__);
	}

	GError* gerr = NULL; // must be initialized
	gsize bytesWritten = 0;
	
	GIOStatus status = g_io_channel_write_chars(m_channel, src, count, &bytesWritten, &gerr);
	
	switch(status) {
	case G_IO_STATUS_NORMAL:
		return bytesWritten;
	case G_IO_STATUS_EOF:
		ErrorOrEOF();
		throw MailNetworkDisconnectionException("unexpected EOF in write", __FILE__, __LINE__);
	case G_IO_STATUS_AGAIN:
		return 0;
	case G_IO_STATUS_ERROR:
		if(gerr) {
			GErrorException exc(gerr, __FILE__, __LINE__);
			g_error_free(gerr);
			SetException(exc);

			ErrorOrEOF();
			throw exc;
		} else {
			// shouldn't ever get here
			throw MailException("unknown write error", __FILE__, __LINE__);
		}
	}
	
	return bytesWritten;
}

void GIOChannelWrapper::SetWatchReadable(bool watch)
{
	if(watch) {
		if(m_watchReadId == 0) {
			m_watchReadId = g_io_add_watch(m_channel, G_IO_IN, &GIOChannelWrapper::ChannelCallback, gpointer(this));
		}
	} else {
		if(m_watchReadId > 0) {
			g_source_remove(m_watchReadId);
			m_watchReadId = 0;
		}
	}
}

void GIOChannelWrapper::SetWatchWriteable(bool watch)
{
	if(watch) {
		if(m_watchWriteId == 0) {
			m_watchWriteId = g_io_add_watch(m_channel, G_IO_OUT, &GIOChannelWrapper::ChannelCallback, gpointer(this));
		}
	} else {
		if(m_watchWriteId > 0) {
			g_source_remove(m_watchWriteId);
			m_watchWriteId = 0;
		}
	}
}

void GIOChannelWrapper::ErrorOrEOF()
{
	Shutdown();
}

gboolean GIOChannelWrapper::ChannelCallback(GIOChannel* channel, GIOCondition cond, gpointer data)
{
	assert(data);
	
	GIOChannelWrapper *asyncChannel = reinterpret_cast<GIOChannelWrapper *>(data);
	
	// Create a temporary reference so the channel doesn't get deleted while we're using it
	MojRefCountedPtr<GIOChannelWrapper> temp(asyncChannel);

	int fd = g_io_channel_unix_get_fd (channel);

	bool keepCallback = false;

	if(cond & G_IO_IN) {
		//fprintf(stderr, "* Channel %d readable, %d readers\n", fd, asyncChannel->m_readers);
		asyncChannel->m_readableSignal.call();
		
		// Don't keep callback if nobody is reading
		keepCallback = asyncChannel->m_readableSignal.connected();
	}

	if(cond & G_IO_OUT) {
		//fprintf(stderr, "* Channel %d writeable, %d writers\n", fd, asyncChannel->m_writers);
		asyncChannel->m_writeableSignal.call();
		
		// Don't keep callback if nobody is writing
		keepCallback = asyncChannel->m_writeableSignal.connected();
	}

	if(cond & G_IO_ERR || cond & G_IO_HUP || cond & G_IO_NVAL) {
		if(cond & G_IO_ERR) {
			fprintf(stderr, "* Channel %d error\n", fd);
		}

		if(cond & G_IO_HUP) {
			fprintf(stderr, "* Channel %d HUP\n", fd);
		}

		if(cond & G_IO_NVAL) {
			fprintf(stderr, "* Channel %d bad fd\n", fd);
		}

		asyncChannel->ErrorOrEOF();
		keepCallback = false;
	}
	
	return keepCallback;
}

void GIOChannelWrapper::Shutdown()
{
	if(m_channel && !m_closed) {
		fprintf(stderr, "* Shutting down channel %d\n", GetFD());
	}

	if(m_watchReadId) {
		g_source_remove(m_watchReadId);
		m_watchReadId = 0;
	}

	if(m_watchWriteId) {
		g_source_remove(m_watchWriteId);
		m_watchWriteId = 0;
	}
	
	if(m_channel && !m_closed) {
		m_closed = true;
		
		try {
			GError *gerr = NULL;
			g_io_channel_shutdown(m_channel, true, &gerr);
			GErrorToException(gerr);
		} catch(const exception& e) {
			// FIXME handle error
			MojLogWarning(s_log, "exception in shutdown: %s", e.what());
		}

		// FIXME -- shouldn't this be handled by MojSignalHander?
		InputStreamPtr temp1 = m_inputStream;
		OutputStreamPtr temp2 = m_outputStream;

		m_closedSignal.fire();
	}
}

const InputStreamPtr& GIOChannelWrapper::GetInputStream()
{
	if(!m_inputStream.get()) {
		m_inputStream.reset( new AsyncInputStream(this) );
	}
	return m_inputStream;
}

const OutputStreamPtr& GIOChannelWrapper::GetOutputStream()
{
	if(!m_outputStream.get()) {
		m_outputStream.reset( new AsyncOutputStream(this) );
	}
	return m_outputStream;
}

GIOChannelWrapperFactory::GIOChannelWrapperFactory()
{	
}

GIOChannelWrapperFactory::~GIOChannelWrapperFactory()
{
}

MojRefCountedPtr<AsyncIOChannel> GIOChannelWrapperFactory::OpenFile(const char* filename, const char* mode)
{
	return GIOChannelWrapper::CreateFileIOChannel(filename, mode);
}

int GIOChannelWrapper::GetFD() const
{
	if(m_channel) {
		return g_io_channel_unix_get_fd(m_channel);
	} else {
		return -1;
	}
}

void GIOChannelWrapper::Status(MojObject& status) const
{
	MojErr err;
	if(m_channel) {
		err = status.put("channelFd", GetFD());
		ErrorToException(err);
	}

	if(m_closed) {
		err = status.put("closed", m_closed);
		ErrorToException(err);
	}

	err = status.put("numReaders", m_readers);
	ErrorToException(err);
	err = status.put("numWriters", m_writers);
	ErrorToException(err);

	if(m_readers > 0 || m_watchReadId > 0) {
		err = status.put("watchReadId", (MojInt64) m_watchReadId);
		ErrorToException(err);
	}

	if(m_writers > 0 || m_watchWriteId > 0) {
		err = status.put("watchWriteId", (MojInt64) m_watchWriteId);
		ErrorToException(err);
	}
}
