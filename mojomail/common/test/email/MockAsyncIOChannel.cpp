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

#include "email/MockAsyncIOChannel.h"
#include "exceptions/MailException.h"
#include "async/AsyncInputStream.h"
#include "async/AsyncOutputStream.h"

using namespace std;

void MockAsyncIOChannel::WatchReadable(ReadableSignal::SlotRef slot)
{
	m_readableSignal.connect(slot);
	
	// Call immediately since we're always ready
	m_readableSignal.call();
}

void MockAsyncIOChannel::UnwatchReadable(ReadableSignal::SlotRef slot)
{
	slot.cancel();
}

void MockAsyncIOChannel::WatchWriteable(WriteableSignal::SlotRef slot)
{
	m_writeableSignal.connect(slot);
	
	// Call immediately since we're always ready
	m_writeableSignal.call();
}

void MockAsyncIOChannel::UnwatchWriteable(WriteableSignal::SlotRef slot)
{
	slot.cancel();
}

void MockAsyncIOChannel::WatchClosed(WriteableSignal::SlotRef slot)
{
	m_closedSignal.connect(slot);
}

void MockAsyncIOChannel::UnwatchClosed(WriteableSignal::SlotRef slot)
{
	slot.cancel();
}



size_t MockAsyncIOChannel::Read(char* dst, size_t length, bool& eof)
{
	if(m_readData.empty()) {
		eof = true;
		return 0;
	}
	
	size_t copied = m_readData.copy(dst, length);
	m_readData.erase(0, copied);
	
	if (m_readData.empty()) {
		eof = true;
	}

	return copied;
}

size_t MockAsyncIOChannel::Write(const char* src, size_t length)
{
	m_writeData.append(src, length);

	return length;
}

void MockAsyncIOChannel::Shutdown()
{
	fprintf(stderr, "shutdown mode %s\n", m_mode.c_str());
	if (m_mode == "w") {
		m_factory->SetFileData(m_fileName, m_writeData);
	}

	m_closedSignal.fire();
}

const InputStreamPtr& MockAsyncIOChannel::GetInputStream()
{
	if(m_inputStream.get() == NULL)
		m_inputStream.reset(new AsyncInputStream(this));
	return m_inputStream;
}

const OutputStreamPtr& MockAsyncIOChannel::GetOutputStream()
{
	if(m_outputStream.get() == NULL)
		m_outputStream.reset(new AsyncOutputStream(this));
	return m_outputStream;
}

void MockAsyncIOChannelFactory::SetFileData(const std::string& fileName, const std::string& data)
{
	m_fileData[fileName] = data;
}

string MockAsyncIOChannelFactory::GetWrittenData(const std::string& fileName)
{
	return m_fileData[fileName];
}

MojRefCountedPtr<AsyncIOChannel> MockAsyncIOChannelFactory::OpenFile(const char* fileName, const char* mode)
{
	if (string(mode) == "r") {
		if(m_fileData.find(fileName) != m_fileData.end()) {
			MojRefCountedPtr<MockAsyncIOChannel> channel( new MockAsyncIOChannel(this, fileName, mode) );
			channel->SetReadData(m_fileData[fileName]);
			return channel;
		} else {
			throw MailException("No such file", __FILE__, __LINE__); // FIXME more specific exception
		}
	} else {
		MojRefCountedPtr<MockAsyncIOChannel> channel( new MockAsyncIOChannel(this, fileName, mode) );
		return channel;
	}
}
