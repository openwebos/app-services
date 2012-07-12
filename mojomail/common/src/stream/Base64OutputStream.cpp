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

#include "stream/Base64OutputStream.h"
#include <cstdio>

// Number of bytes to convert at a time
size_t Base64EncoderOutputStream::BLOCK_SIZE = 4096;

Base64EncoderOutputStream::Base64EncoderOutputStream(const OutputStreamPtr& sink)
: ChainedOutputStream(sink), m_state(0), m_save(0)
{
	size_t bufsize = ((BLOCK_SIZE / 3 + 1) * 4 + 4);
	
	bufsize += 2 * (bufsize / 72 + 1); // add space for CRLF terminators
	
	m_outbuf.reset( new char[bufsize] );
}

Base64EncoderOutputStream::~Base64EncoderOutputStream()
{
}

guint Base64EncoderOutputStream::FixLineEndings(char * buffer, size_t len)
{
	// HACK: Fix line endings. This should be done with a custom encoder, instead.
	char * n = buffer;
	char * end = n + len;
	while ((n <= end) && (n = (char*)memchr(n, '\n', end-n))) {
		memmove(n+1, n, end-n);
		*n = '\r';
		n += 2; // move past \r\n
		len++;
		end++;
	}
	return len;
}

void Base64EncoderOutputStream::Write(const char* src, size_t length)
{
	char *outbuf = m_outbuf.get();
	
	for(size_t offset = 0; offset < length; offset += BLOCK_SIZE) {
		const unsigned char* start = (const unsigned char*) src + offset;
		size_t remaining = length - offset;
		size_t blocklen = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;
		
		// FIXME need \r\n not \n newlines
		gsize numBytesOut = g_base64_encode_step(start, blocklen, true, outbuf, &m_state, &m_save);
		
		numBytesOut = FixLineEndings(outbuf, numBytesOut);
		
		m_sink->Write(outbuf, numBytesOut);
	}
}

void Base64EncoderOutputStream::Flush(FlushType flushType)
{
	char *outbuf = m_outbuf.get();
	gsize numBytesOut = g_base64_encode_close(true, outbuf, &m_state, &m_save);

	numBytesOut = FixLineEndings(outbuf, numBytesOut);

	m_sink->Write(outbuf, numBytesOut);
	m_sink->Flush(flushType);
}
