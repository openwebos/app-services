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

#ifndef BYTEBUFFER_H_
#define BYTEBUFFER_H_

#include <string>
#include <sys/types.h>

#include "core/MojCoreDefs.h"
#include "core/MojSignal.h"
#include "stream/BaseOutputStream.h"

class ByteBufferOutputStream : public BaseOutputStream
{
public:
	ByteBufferOutputStream(size_t softSizeLimit = 8192);
	virtual ~ByteBufferOutputStream();
	
	typedef MojSignal<> FullSignal;  // please stop writing new data
	typedef MojSignal<> ReadableSignal; // data ready to read
	typedef MojSignal<> WriteableSignal; // space available to write
	
public: // BaseOutputStream methods
	void Write(const char *src, size_t length) {
		m_buffer.append(src, length);

		// If the buffer is full (past soft limit) call full signal
		if(IsFull())
			m_fullSignal.call();
		
		// If it was empty before, call readable signal
		if(length > 0 && m_buffer.size() == length)
			m_readableSignal.call();
	}
	
	void Flush(FlushType flushType = FullFlush) {
		m_readableSignal.call();
	}
	
	void Close() {
		// does nothing
	}

public: // ByteBuffer-specific methods
	void SetFullSlot(FullSignal::SlotRef slot) {
		m_fullSignal.connect(slot);
	}
	
	void SetReadableSlot(ReadableSignal::SlotRef slot) {
		m_readableSignal.connect(slot);
	}
	
	void SetWriteableSlot(WriteableSignal::SlotRef slot) {
		m_writeableSignal.connect(slot);
	}
	
	size_t ReadFromBuffer(char *dst, size_t length) {
		size_t copied = m_buffer.copy(dst, length);
		m_buffer.erase(0, copied);
		
		if(IsEmpty())
			m_writeableSignal.call();
		
		return copied;
	}
	
	void Clear()
	{
		m_buffer.clear();
	}

	const std::string& GetBuffer()
	{
		return m_buffer;
	}

	inline bool IsFull() {
		return m_buffer.size() >= m_softSizeLimit;
	}
	
	inline bool IsEmpty() {
		return m_buffer.size() == 0;
	}
	
protected:
	size_t m_softSizeLimit;
	std::string m_buffer;
	
	// Signals
	FullSignal m_fullSignal;
	ReadableSignal m_readableSignal;
	WriteableSignal m_writeableSignal;
};

#endif /*BYTEBUFFER_H_*/
