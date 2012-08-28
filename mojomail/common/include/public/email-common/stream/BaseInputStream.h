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

#ifndef BASEINPUTSTREAM_H_
#define BASEINPUTSTREAM_H_

#include <sys/types.h>
#include "CommonDefs.h"
#include "core/MojSignal.h"

class BaseInputStream;

/**
 * A receiver for incoming data
 */
class InputStreamSink : public virtual MojSignalHandler
{
public:
	InputStreamSink(const MojRefCountedPtr<BaseInputStream>& source);
	virtual ~InputStreamSink();

	/**
	 * Handle incoming data. If not all of the data is handled, it will get
	 * passed again in the next call to handleData.
	 * 
	 * @param eof no more data will be available from this stream
	 * @return number of bytes handled
	 */
	virtual size_t	HandleData(const char* data, size_t length, bool eof) = 0;

	/**
	 * Disconnects a sink from the source, meaning another sink is using the source.
	 */
	virtual void	DisconnectFromSource();

protected:
	MojRefCountedPtr<BaseInputStream>	m_source;
};

/**
 * BaseInputStream is a stream which processes data passed in using handleData
 * and passes the processed data (decoded, decompressed, etc) to
 * the specified sink.
 */
class BaseInputStream : public InputStreamSink
{
public:
	BaseInputStream(const MojRefCountedPtr<BaseInputStream>& source = MojRefCountedPtr<BaseInputStream>(NULL)) : InputStreamSink(source) {}
	virtual ~BaseInputStream() {}
	
	/**
	 * Called to trigger a read, or start waiting for data to be available.
	 * Used to indicate that the sink input stream needs more data.
	 * OK to call even if already reading.
	 */
	virtual void StartReading() = 0;
	
	/**
	 * Indicates that the sink input stream doesn't need more data.
	 */
	virtual void DoneReading() = 0;
	
	/**
	 * Sets the stream's sink, which should be called with any data processed
	 * by this input stream.
	 */
	virtual void SetSink(InputStreamSink* sink) = 0;

	/**
	 * Removes the given sink
	 */
	virtual void RemoveSink(InputStreamSink* sink) = 0;
};

typedef MojRefCountedPtr<BaseInputStream> InputStreamPtr;

/**
 * A base class that implements some common methods related to chaining streams.
 */
class ChainableInputStream : public BaseInputStream
{
public:
	ChainableInputStream(const MojRefCountedPtr<BaseInputStream>& source = MojRefCountedPtr<BaseInputStream>(NULL));
	virtual ~ChainableInputStream();

	virtual void StartReading();
	virtual void DoneReading();
	
	virtual void SetSink(InputStreamSink* sink);
	virtual void RemoveSink(InputStreamSink* sink);
	
protected:
	InputStreamSink* m_sink;
};

#endif /*BASEINPUTSTREAM_H_*/
