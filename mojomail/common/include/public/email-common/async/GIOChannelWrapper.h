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

#ifndef GIOCHANNELWRAPPER_H_
#define GIOCHANNELWRAPPER_H_

#include <glib.h>
#include "CommonDefs.h"
#include "core/MojSignal.h"
#include "async/AsyncIOChannel.h"
#include "stream/BaseOutputStream.h"
#include "stream/BaseInputStream.h"

/**
 * /brief
 * Wrapper for GIOChannel
 */
class GIOChannelWrapper : public AsyncIOChannel
{
public:
	// Open an IO channel for a file
	static MojRefCountedPtr<AsyncIOChannel> CreateFileIOChannel(const char* filename, const char* mode);

	virtual void Shutdown();
	
	virtual size_t Read(char* dst, size_t count, bool& eof);
	virtual size_t Write(const char* src, size_t count);
	
	virtual const InputStreamPtr&		GetInputStream();
	virtual const OutputStreamPtr&		GetOutputStream();
	
	int GetFD() const;
	virtual void Status(MojObject& status) const;

protected:
	GIOChannelWrapper(GIOChannel* channel);
	virtual ~GIOChannelWrapper();
	
	static gboolean ChannelCallback(GIOChannel* channel, GIOCondition cond, gpointer data);
	
	virtual void ErrorOrEOF();

	virtual void SetWatchReadable(bool watch);
	virtual void SetWatchWriteable(bool watch);

	GIOChannel*		m_channel;
	int				m_readers;
	int				m_writers;
	guint			m_watchReadId;
	guint			m_watchWriteId;
	bool			m_closed;

	boost::exception_ptr	m_exception;
};

class GIOChannelWrapperFactory : public AsyncIOChannelFactory
{
public:
	GIOChannelWrapperFactory();
	virtual ~GIOChannelWrapperFactory();
	
	virtual MojRefCountedPtr<AsyncIOChannel> OpenFile(const char* filename, const char* mode);
};

#endif /*GIOCHANNELWRAPPER_H_*/
