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

#ifndef ASYNCINPUTSTREAM_H_
#define ASYNCINPUTSTREAM_H_

#include "stream/BaseInputStream.h"
#include "async/AsyncIOChannel.h"
#include <string>

class AsyncInputStream : public ChainableInputStream
{
public:
	static const int DEFAULT_BUFFER_SIZE;
	
	AsyncInputStream(AsyncIOChannel* channel);
	virtual ~AsyncInputStream();
	
	size_t	HandleData(const char* data, size_t length, bool eof);
	
protected:
	void	StartReading();
	void	DoneReading();
	
	void	Watch();
	void	Unwatch();
	
	// Reads data from the channel and calls handleData.
	void	ReadFromChannel();
	
	void	ScheduleFlush();

	// Callback when the channel tells us there's data ready to read
	MojErr	ChannelReadable();
	
	MojErr	ChannelClosed();

	// Called to flush the buffer asynchronously
	static gboolean		FlushBufferCallback(gpointer data);
	void				FlushBuffer();
	
	// Error
	void				Error(const std::exception& e);

	AsyncIOChannel*	m_channel;
	
	bool			m_wantData;			// whether we want to read more data
	bool			m_watching;			// whether we're set to receive readable callbacks
	guint			m_callbackId;		// callback scheduled to flush the buffer
	bool			m_eof;				// no more data can be read from the channel
	std::string		m_buffer;
	
	AsyncIOChannel::ReadableSignal::Slot<AsyncInputStream> m_readableSlot;
	AsyncIOChannel::ClosedSignal::Slot<AsyncInputStream> m_closedSlot;
};

#endif /*ASYNCINPUTSTREAM_H_*/
