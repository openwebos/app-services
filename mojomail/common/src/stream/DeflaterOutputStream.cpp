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

#include "stream/DeflaterOutputStream.h"
#include "exceptions/MailException.h"

const int DeflaterOutputStream::BUFFER_SIZE = 8192;

DeflaterOutputStream::DeflaterOutputStream(const OutputStreamPtr& sink, int windowBits, int compressLevel)
: ChainedOutputStream(sink)
{
	// Setup zalloc, zfree, and opaque
	m_stream.zalloc = Z_NULL;
	m_stream.zfree = Z_NULL;
	m_stream.opaque = Z_NULL;

	m_stream.next_in = Z_NULL;

	if(deflateInit2(&m_stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowBits, compressLevel, Z_DEFAULT_STRATEGY) != Z_OK) {
		throw MailException("zlib stream init failed", __FILE__, __LINE__);
	}
}

DeflaterOutputStream::~DeflaterOutputStream()
{
	deflateEnd(&m_stream);
}

void DeflaterOutputStream::Write(const char* src, size_t length)
{
	const int bufSize = BUFFER_SIZE;
	unsigned char outbuf[bufSize];

	m_stream.next_in = (unsigned char*) src;
	m_stream.avail_in = length;

	do {
		m_stream.next_out = outbuf;
		m_stream.avail_out = bufSize;

		int ret = deflate(&m_stream, Z_NO_FLUSH);

		if(ret == Z_OK) {
			m_sink->Write((const char*) outbuf, bufSize - m_stream.avail_out);
		} else if(ret == Z_BUF_ERROR) {
			// No more input available; some output may be buffered
			break;
		} else {
			throw MailException("error deflating", __FILE__, __LINE__);
		}
	} while(m_stream.avail_in > 0);
}

void DeflaterOutputStream::Flush(FlushType flushType)
{
	const int bufSize = BUFFER_SIZE;
	unsigned char buf[bufSize];

	m_stream.next_in = (unsigned char*) "";
	m_stream.avail_in = 0;

	m_stream.next_out = buf;
	m_stream.avail_out = bufSize;

	if(deflate(&m_stream, Z_SYNC_FLUSH) == Z_OK) {
		m_sink->Write((const char*) buf, bufSize - m_stream.avail_out);
	} else {
		throw MailException("error flushing deflate stream", __FILE__, __LINE__);
	}

	ChainedOutputStream::Flush(flushType);
}

void DeflaterOutputStream::GetDeflateStats(DeflateStats& stats)
{
	stats.bytesIn = m_stream.total_in;
	stats.bytesOut = m_stream.total_out;
}
