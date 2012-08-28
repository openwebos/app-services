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

#ifndef INFLATERINPUTSTREAM_H_
#define INFLATERINPUTSTREAM_H_

#include "stream/BaseInputStream.h"
#include <zlib.h>
#include <string>
#include <glib.h>

class InflaterInputStream : public ChainableInputStream
{
public:
	// Note: memory usage is defined in the zlib manual as:
	// inflate memory usage (bytes) = (1 << windowBits) + 1440*2*sizeof(int)
	InflaterInputStream(const InputStreamPtr& inputStream, int windowBits = -15);
	virtual ~InflaterInputStream();

	virtual void	StartReading();
	virtual size_t	HandleData(const char* data, size_t length, bool eof);

	struct InflateStats {
		InflateStats() : bytesIn(0), bytesOut(0) {}

		MojInt64 bytesIn;
		MojInt64 bytesOut;
	};

	virtual void GetInflateStats(InflateStats& stats);

protected:
	static const int BUFFER_SIZE;

	static gboolean FlushBufferCallback(gpointer data);
	void FlushBuffer();

	z_stream	m_stream;
	std::string	m_buffer;
	bool		m_eof;

	int			m_callbackId;
};

#endif /* INFLATERINPUTSTREAM_H_ */
