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

#include "stream/Base64DecoderOutputStream.h"
#include <cstdio>

// Number of bytes to convert at a time
size_t Base64DecoderOutputStream::BLOCK_SIZE = 4096;

Base64DecoderOutputStream::Base64DecoderOutputStream(const OutputStreamPtr& sink)
: ChainedOutputStream(sink), m_state(0), m_save(0)
{
	//FIXME the buffer size should be smaller
	size_t bufsize = ((BLOCK_SIZE / 3 + 1) * 4 + 4);

	bufsize += bufsize / 72 + 1; // add space for newlines

	m_outbuf.reset( new char[bufsize] );
}

Base64DecoderOutputStream::~Base64DecoderOutputStream()
{
}

void Base64DecoderOutputStream::Write(const char* src, size_t length)
{
	unsigned char *outbuf = (unsigned char *)m_outbuf.get();

	for(size_t offset = 0; offset < length; offset += BLOCK_SIZE) {
		const char* start = src + offset;
		size_t remaining = length - offset;
		size_t blocklen = remaining < BLOCK_SIZE ? remaining : BLOCK_SIZE;

		// FIXME need \r\n not \n newlines
		gsize numBytesOut = g_base64_decode_step(start, blocklen, outbuf, &m_state, &m_save);
		m_sink->Write((const char*)outbuf, numBytesOut);
	}
}

void Base64DecoderOutputStream::Flush()
{
	m_sink->Flush();
}
