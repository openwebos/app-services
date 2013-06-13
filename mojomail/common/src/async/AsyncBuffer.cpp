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

#include "AsyncBuffer.h"

#if 0 // FIXME doesn't really work yet

AsyncBuffer::AsyncBuffer(BufferPtr buffer)
: m_buffer(buffer),
  m_bufferReadableSlot(this, &AsyncBuffer::BufferReadable),
  m_bufferWriteableSlot(this, &AsyncBuffer::BufferWriteable),
  m_bufferFullSlot(this, &AsyncBuffer::BufferFull),
{
	buffer->SetReadableSlot(m_bufferReadableSlot);
	buffer->SetWriteableSlot(m_bufferWriteableSlot);
	buffer->SetFullSlot(m_bufferFullSlot);
}

AsyncBuffer::~AsyncBuffer()
{
}

// Called when the buffer has bytes ready to read
// May be delayed until a flush or when the buffer fills up
MojErr AsyncBuffer::BufferReadable()
{
	if(m_readerThrottled) {
		MojLogInfo(s_log, "unthrottling connection");
		ResumeReading();
		m_connectionThrottled = false;
	}
	return MojErrNone;
}

// Called when the buffer has space
// May be delayed until the buffer is empty
MojErr AsyncBuffer::BufferWriteable()
{
	// TODO: resume reading part data from file
	if(m_writerThrottled) {
		m_writerThrottled = false;
		MojLogInfo(s_log, "buffer writeable, resuming AsyncEmailWriter");
		m_emailWriter->ResumeWriting();
	}
	return MojErrNone;
}

MojErr AsyncBuffer::BufferFull()
{
	// NOTE: BufferReadable() will usually get called immediately after this
	
	if(!m_writerThrottled) {
		m_writerThrottled = true;
		MojLogInfo(s_log, "buffer full, pausing AsyncEmailWriter");
		m_emailWriter->PauseWriting();
	}
	
	return MojErrNone;
}
#endif
