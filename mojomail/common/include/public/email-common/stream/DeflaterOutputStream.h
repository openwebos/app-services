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

#ifndef DEFLATEROUTPUTSTREAM_H_
#define DEFLATEROUTPUTSTREAM_H_

#include "stream/BaseOutputStream.h"
#include <zlib.h>

class DeflaterOutputStream : public ChainedOutputStream
{
public:
	// Note: memory usage is defined in the zlib manual as:
	// deflate memory usage (bytes) = (1 << (windowBits+2)) + (1 << (memLevel+9))
	DeflaterOutputStream(const OutputStreamPtr& sink, int windowBits = -8, int compressLevel = 8);
	virtual ~DeflaterOutputStream();

	virtual void Write(const char* src, size_t length);
	virtual void Flush(FlushType flushType = FullFlush);

	struct DeflateStats {
		DeflateStats() : bytesIn(0), bytesOut(0) {}

		MojInt64 bytesIn;
		MojInt64 bytesOut;
	};

	virtual void GetDeflateStats(DeflateStats& stats);

protected:
	static const int BUFFER_SIZE;

	z_stream				m_stream;
};

#endif /* DEFLATEROUTPUTSTREAM_H_ */
