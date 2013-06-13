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

#ifndef ASYNCBUFFER_H_
#define ASYNCBUFFER_H_

#include "CommonDefs.h"
#include "core/MojSignal.h"

class AsyncBuffer
{
	typedef MojRefCountedPtr<ByteBufferOutputStream> BufferPtr;
	
public:
	AsyncBuffer(BufferPtr stream);
	virtual ~AsyncBuffer();
	
protected:
	BufferPtr m_buffer;
	
	MojErr	BufferReadable();
	MojErr	BufferWriteable();
	MojErr	BufferFull();
	
	// Buffer slots
	ByteBufferOutputStream::FullSignal::Slot<AsyncBuffer> m_bufferReadableSlot;
	ByteBufferOutputStream::FullSignal::Slot<AsyncBuffer> m_bufferWriteableSlot;
	ByteBufferOutputStream::FullSignal::Slot<AsyncBuffer> m_bufferFullSlot;
};

#endif /*ASYNCBUFFER_H_*/
