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

#ifndef MOCKSTREAMS_H_
#define MOCKSTREAMS_H_

#include "stream/BaseInputStream.h"
#include "stream/ByteBufferOutputStream.h"
#include "exceptions/MailException.h"

class MockInputStream : public BaseInputStream
{
public:
	MockInputStream() : m_sink(NULL), m_eof(false) {}
	virtual ~MockInputStream() {}

	// BaseInputStream method implementations
	virtual void StartReading() {}
	virtual void DoneReading() {}
	virtual void SetSink(InputStreamSink* sink) { m_sink = sink; }
	virtual void RemoveSink(InputStreamSink* sink) { if(m_sink == sink) m_sink = NULL; }

	virtual size_t HandleData(const char*data, size_t size, bool eof)
	{
		throw MailException("MockInputStream: unexpected HandleData call", __FILE__, __LINE__);
	}

	// Used by tests to provide data
	virtual void Feed(const std::string& data)
	{
		// Add to buffer
		m_buffer.append(data);

		if(m_sink) {
			size_t bytesHandled = m_sink->HandleData(m_buffer.data(), m_buffer.size(), m_eof);
			m_buffer.erase(0, bytesHandled);
		}
	}

	virtual void FlushBuffer()
	{
		for(int i = 0; i < 100 && !m_buffer.empty(); i++) {
			Feed("");
		}
	}

	virtual void FeedLine(const std::string& data) { Feed(data + "\r\n"); }

protected:
	InputStreamSink*	m_sink;
	std::string			m_buffer;
	bool				m_eof;
};

class MockInputStreamReader : public InputStreamSink
{
public:
	MockInputStreamReader(const InputStreamPtr& source) : InputStreamSink(source) {}

	virtual size_t HandleData(const char* data, size_t size, bool eof)
	{
		m_buffer.append(data, size);

		return size;
	}

	virtual const std::string& GetBuffer() const { return m_buffer; }
	virtual void ClearBuffer() { m_buffer.clear(); }

protected:
	std::string		m_buffer;
};

class MockOutputStream : public ByteBufferOutputStream
{
public:
	virtual std::string GetLine()
	{
		size_t pos = m_buffer.find("\r\n");
		if(pos != std::string::npos) {
			std::string line = m_buffer.substr(0, pos);
			m_buffer.erase(0, pos+2);
			return line;
		} else {
			throw MailException("MockOutputStream: no line available", __FILE__, __LINE__);
		}
	}
};

typedef MojRefCountedPtr<MockInputStream> MockInputStreamPtr;
typedef MojRefCountedPtr<MockOutputStream> MockOutputStreamPtr;

#endif /* MOCKSTREAMS_H_ */
