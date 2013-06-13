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

#include "stream/InflaterInputStream.h"
#include "exceptions/MailException.h"
#include "CommonPrivate.h"

const int InflaterInputStream::BUFFER_SIZE = 8192;

InflaterInputStream::InflaterInputStream(const InputStreamPtr& source, int windowBits)
: ChainableInputStream(source),
  m_eof(false),
  m_callbackId(0)
{
	m_stream.zalloc = Z_NULL;
	m_stream.zfree = Z_NULL;
	m_stream.opaque = Z_NULL;

	m_stream.next_in = Z_NULL;

	if(inflateInit2(&m_stream, windowBits) != Z_OK) {
		throw MailException("error initializing inflater stream", __FILE__, __LINE__);
	}
}

InflaterInputStream::~InflaterInputStream()
{
	inflateEnd(&m_stream);

	if(m_callbackId) {
		g_source_remove(m_callbackId);
	}
}

void InflaterInputStream::StartReading()
{
	if(m_source.get()) {
		m_source->StartReading();
	}

	if(m_callbackId == 0) {
		m_callbackId = g_idle_add(&InflaterInputStream::FlushBufferCallback, gpointer(this));
	}
}

size_t InflaterInputStream::HandleData(const char* data, size_t length, bool eof)
{
	m_stream.next_in = (unsigned char*) data;
	m_stream.avail_in = length;

	// Inflate as much as possible
	do {
		const int bufSize = BUFFER_SIZE;
		unsigned char buf[bufSize];

		m_stream.next_out = buf;
		m_stream.avail_out = bufSize;

		int ret = inflate(&m_stream, Z_SYNC_FLUSH);

		if(ret == Z_OK) {
			m_buffer.append((const char*) buf, bufSize - m_stream.avail_out);
		} else if(ret == Z_BUF_ERROR) {
			// can't inflate any more right now
			break;
		} else {
			throw MailException("error inflating", __FILE__, __LINE__);
		}
	} while(m_stream.avail_out == 0);

	if(m_sink) {
		//fprintf(stderr, "total in: %ld, total out: %ld, savings: %ldKB\n", m_stream.total_in, m_stream.total_out, (m_stream.total_out - m_stream.total_in)/1024);

		// Make sure the sink doesn't get deleted until after HandleData returns
		MojRefCountedPtr<InputStreamSink> temp(m_sink);

		size_t bufferBytesHandled = m_sink->HandleData(m_buffer.data(), m_buffer.size(), eof);
		m_buffer.erase(0, bufferBytesHandled);
	}

	m_eof = eof;

	return length - m_stream.avail_in; // bytes handled
}

gboolean InflaterInputStream::FlushBufferCallback(gpointer data)
{
	InflaterInputStream* stream = reinterpret_cast<InflaterInputStream*>(data);
	assert(stream);

	// Hold temp ref so it doesn't get deleted
	MojRefCountedPtr<InflaterInputStream> temp(stream);

	stream->FlushBuffer();

	return false;
}

void InflaterInputStream::FlushBuffer()
{
	m_callbackId = 0;

	if(m_sink && !m_buffer.empty()) {
		HandleData("", 0, m_eof);
	}
}

void InflaterInputStream::GetInflateStats(InflateStats& stats)
{
	stats.bytesIn = m_stream.total_in;
	stats.bytesOut = m_stream.total_out;
}
