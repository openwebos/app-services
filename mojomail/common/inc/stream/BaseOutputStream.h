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

#ifndef BASEOUTPUTSTREAM_H_
#define BASEOUTPUTSTREAM_H_

#include <sys/types.h>
#include "CommonDefs.h"
#include "core/MojSignal.h"
#include <string>
#include "CommonErrors.h"

class BaseOutputStream : public MojSignalHandler
{
public:
	enum FlushType
	{
		FullFlush,		// Write everything to the underlying file/socket
		PartialFlush,	// Flush only intermediate streams
		CloseFlush		// Flush that occurs as part of a close
	};

	BaseOutputStream() {}
	virtual ~BaseOutputStream() {}
	
	// Convenience method (non-virtual)
	void Write(std::string src) { Write(src.data(), src.length()); }
	
	// Write some data to the stream
	virtual void Write(const char* src, size_t length) = 0;
	
	// Flush the stream, including any chained streams
	// If partial is set to true, only intermediate streams will be flushed,
	// not the underlying file/socket.
	virtual void Flush(FlushType flushType = FullFlush) = 0;

	// Close the stream after all pending data is flushed
	virtual void Close() = 0;

	virtual bool HasError() { return GetError().errorCode != MailError::NONE; }
	virtual MailError::ErrorInfo GetError() const { return m_error; }

protected:
	MailError::ErrorInfo	m_error;
};

typedef MojRefCountedPtr<BaseOutputStream> OutputStreamPtr;

/**
 * Convenience class for an output stream that writes to another output stream.
 */
class ChainedOutputStream : public BaseOutputStream
{
public:
	ChainedOutputStream(const OutputStreamPtr& sink) : m_sink(sink) {}
	virtual ~ChainedOutputStream() {}

	// Overrides BaseOutputStream
	inline virtual void Write(const char* src, size_t length)
	{
		m_sink->Write(src, length);
	}

	// Overrides BaseOutputStream
	inline virtual void Flush(FlushType flushType = FullFlush)
	{
		m_sink->Flush(flushType);
	}

	// Overrides BaseOutputStream
	inline virtual void Close()
	{
		m_sink->Close();
	}

	virtual MailError::ErrorInfo GetError() const {
		if (m_error.errorCode != MailError::NONE) {
			return m_error;
		} else if (m_sink.get()) {
			return m_sink->GetError();
		} else {
			return MailError::ErrorInfo(MailError::NONE);
		}
	}

protected:
	OutputStreamPtr m_sink;
};

#endif /*BASEOUTPUTSTREAM_H_*/
