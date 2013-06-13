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

#ifndef BASE64OUTPUTSTREAM_H_
#define BASE64OUTPUTSTREAM_H_

#include <glib.h>
#include "stream/BaseOutputStream.h"
#include <boost/shared_array.hpp>

class Base64EncoderOutputStream : public ChainedOutputStream
{
	static size_t BLOCK_SIZE;
	
public:
	Base64EncoderOutputStream(const OutputStreamPtr& sink);
	virtual ~Base64EncoderOutputStream();

	// Write some data to the stream
	// Overrides BaseOutputStream
	virtual void Write(const char* src, size_t length);

	// Flush the stream, including any chained streams
	// Overrides BaseOutputStream
	virtual void Flush(FlushType flushType = FullFlush);

protected:
	guint FixLineEndings(char * buffer, size_t len);

	boost::shared_array<char>	m_outbuf;
	gint						m_state;
	gint						m_save;
};

#endif /*BASE64OUTPUTSTREAM_H_*/
