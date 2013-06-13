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

#ifndef MOCKASYNCIOCHANNEL_H_
#define MOCKASYNCIOCHANNEL_H_

#include "async/AsyncIOChannel.h"
#include <string>
#include <map>

class MockAsyncIOChannelFactory;

class MockAsyncIOChannel : public AsyncIOChannel
{
public:
	MockAsyncIOChannel(MockAsyncIOChannelFactory* factory, const char* fileName, const char* mode)
	: m_fileName(fileName), m_mode(mode), m_factory(factory) {}
	virtual ~MockAsyncIOChannel() {}
	
	virtual void WatchReadable(ReadableSignal::SlotRef slot);
	virtual void UnwatchReadable(ReadableSignal::SlotRef slot);
	virtual void WatchWriteable(WriteableSignal::SlotRef slot);
	virtual void UnwatchWriteable(WriteableSignal::SlotRef slot);
	virtual void WatchClosed(WriteableSignal::SlotRef slot);
	virtual void UnwatchClosed(WriteableSignal::SlotRef slot);
	
	virtual void Shutdown();
	virtual size_t Read(char* buf, size_t count, bool& eof);
	virtual size_t Write(const char* buf, size_t count);
	
	virtual const InputStreamPtr&	GetInputStream();
	virtual const OutputStreamPtr&	GetOutputStream();
	
	void SetReadData(const std::string& s) { m_readData = s; }

protected:
	std::string			m_fileName;
	std::string			m_mode;
	MockAsyncIOChannelFactory*	m_factory;

	InputStreamPtr		m_inputStream;
	OutputStreamPtr		m_outputStream;
	
	std::string			m_readData;
	std::string			m_writeData;
};

class MockAsyncIOChannelFactory : public AsyncIOChannelFactory
{
public:
	MockAsyncIOChannelFactory() {}
	virtual ~MockAsyncIOChannelFactory() {}
	
	virtual MojRefCountedPtr<AsyncIOChannel> OpenFile(const char* filename, const char* mode);
	
	// Set fake file
	void SetFileData(const std::string& fileName, const std::string& data);
	std::string GetWrittenData(const std::string& fileName);
	
protected:
	std::map<std::string, std::string> m_fileData;
};

#endif /*MOCKASYNCIOCHANNEL_H_*/
