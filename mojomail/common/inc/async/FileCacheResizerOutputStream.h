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

#ifndef FILECACHERESIZEROUTPUTSTREAM_H_
#define FILECACHERESIZEROUTPUTSTREAM_H_

#include "stream/ByteBufferOutputStream.h"
#include "client/FileCacheClient.h"

class FileCacheResizerOutputStream : public ChainedOutputStream
{
public:
	FileCacheResizerOutputStream(const OutputStreamPtr& sink, FileCacheClient& fileCacheClient, const std::string& path, MojInt64 initialSize);
	virtual ~FileCacheResizerOutputStream();

	typedef MojSignal<> FullSignal;  // please stop writing new data
	typedef MojSignal<> WriteableSignal; // space available to write

	void SetFullSlot(FullSignal::SlotRef slot) {
		m_fullSignal.connect(slot);
	}

	void SetWriteableSlot(WriteableSignal::SlotRef slot) {
		m_writeableSignal.connect(slot);
	}

	// Overrides ByteBufferOutputStream
	void Write(const char* src, size_t length);

	// Overrides ByteBufferOutputStream
	void Flush(FlushType flushType = FullFlush);

	// Overrides ByteBufferOutputStream
	void Close();

protected:
	void FlushBuffer();

	void RequestResize();
	MojErr ResizeResponse(MojObject& response, MojErr err);

	MojInt64 SpaceLeft() const;

	FileCacheClient&	m_fileCacheClient;
	std::string			m_path;

	MojInt64	m_fileCacheBytesAllocated;
	MojInt64	m_bytesWritten;
	std::string	m_buffer;
	bool		m_resizeRequestPending;
	bool		m_closePending;

	MojInt64	m_requestedBytes;

	// Signals
	FullSignal m_fullSignal;
	WriteableSignal m_writeableSignal;

	FileCacheClient::ReplySignal::Slot<FileCacheResizerOutputStream>	m_resizeSlot;
};

#endif /* FILECACHERESIZEROUTPUTSTREAM_H_ */
