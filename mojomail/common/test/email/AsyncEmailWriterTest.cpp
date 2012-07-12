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

#include "email/AsyncEmailWriter.h"
#include "data/Email.h"
#include "data/EmailAddress.h"
#include "data/EmailPart.h"
#include "stream/ByteBufferOutputStream.h"
#include "stream/CounterOutputStream.h"
#include "email/MockAsyncIOChannel.h"
#include <sstream>
#include <gtest/gtest.h>

// Helper to watch signal responses
class AsyncEmailWriterResult : public MojSignalHandler
{
	typedef AsyncEmailWriter::EmailWrittenSignal Signal;
public:
	
	AsyncEmailWriterResult()
	: m_count(0),
	  m_hasException(false),
	  m_slot(this, &AsyncEmailWriterResult::Callback)
	{
	}
	
	MojErr Callback(const std::exception* e)
	{
		m_count++;
		if(e) {
			m_hasException = true;
			m_exception = *e;
		}
		return MojErrNone;
	}
	
	bool					Done() { return m_count > 0; }
	const std::exception*	GetException() { return m_hasException ? &m_exception : NULL; }
	Signal::SlotRef			GetSlot() { return m_slot; }
	
protected:
	bool									m_count;
	bool									m_hasException;
	std::exception							m_exception;
	
public:
	Signal::Slot<AsyncEmailWriterResult>	m_slot;
};

// Copy buffer into a string
void ReadBuffer(std::string &s, ByteBufferOutputStream *bbos)
{
	const size_t BUF_SIZE = 8192;
	char buf[BUF_SIZE];
	
	size_t nread;
	while( (nread = bbos->ReadFromBuffer(buf, BUF_SIZE)) > 0) {
		s.append(buf, nread);
	}
}

EmailPartPtr CreateBodyPart(const char* filename)
{
	EmailPartPtr body( new EmailPart(EmailPart::BODY) );
	body->SetLocalFilePath(filename);
	body->SetMimeType("text/html");
	return body;
}

void InitEmail(Email& email)
{
	// Minimal email headers
	EmailAddressPtr from( new EmailAddress("test", "test@example.com") );
	email.SetFrom(from);
	email.SetSubject("Test email");
	email.SetDateReceived(1234567890000LL); // Friday 13 Feb 2009
}

void WriteEmail(MojRefCountedPtr<AsyncEmailWriter> writer, std::string& output)
{
	// Write to a counter output stream
	MojRefCountedPtr<CounterOutputStream> counter( new CounterOutputStream() );
	writer->SetOutputStream(counter);

	MojRefCountedPtr<AsyncEmailWriterResult> writeResult(new AsyncEmailWriterResult());
	writer->WriteEmail(writeResult->GetSlot());

	// Make sure it's done writing
	ASSERT_TRUE( writeResult->Done() );

	// Check if any exceptions were reported
	ASSERT_TRUE( !writeResult->GetException() );

	// Now write it again to a buffer
	writeResult.reset(new AsyncEmailWriterResult());

	MojRefCountedPtr<ByteBufferOutputStream> bbos( new ByteBufferOutputStream() );
	writer->SetOutputStream(bbos);

	writer->WriteEmail(writeResult->GetSlot());

	// Make sure it's done writing
	ASSERT_TRUE( writeResult->Done() );

	// Check if any exceptions were reported
	ASSERT_TRUE( !writeResult->GetException() );
	
	// Read generated email into output string
	ReadBuffer(output, bbos.get());
	
	// Make sure the count matches the actual bytes written
	ASSERT_EQ(counter->GetBytesWritten(), output.size());
}

void TestWriteEmail(int numAttachments)
{
	Email email;
	InitEmail(email);
	
	MojRefCountedPtr<AsyncEmailWriter> writer( new AsyncEmailWriter(email) );
	
	// Create a body part
	EmailPartList partList;
	partList.push_back( CreateBodyPart("test1") );
	
	// Create some attachments
	for(int i = 0; i < numAttachments; i++) {
		std::stringstream name;
		name << "attach" << i;
		
		EmailPartPtr attachmentPart( new EmailPart(EmailPart::ATTACHMENT) );
		attachmentPart->SetLocalFilePath(name.str());
		partList.push_back(attachmentPart);
	}
	writer->SetPartList(partList);
	
	// Set up a fake file "test1" in our mock factory
	boost::shared_ptr<MockAsyncIOChannelFactory> ioFactory( new MockAsyncIOChannelFactory() );
	writer->SetAsyncIOChannelFactory(ioFactory);
	ioFactory->SetFileData("test1", "Email body");
	
	// Make some fake attachments
	for(int i = 0; i < numAttachments; i++) {
		std::stringstream name, data;
		name << "attach" << i;
		data << "Attachment data " << i;
		ioFactory->SetFileData(name.str().c_str(), data.str());
	}
	
	std::string output;
	WriteEmail(writer, output);
	
	//fprintf(stderr, "Email:\n%s\n", output.c_str());
}

TEST(AsyncEmailWriterTest, TestWriteEmail)
{
	TestWriteEmail(0);
}

TEST(AsyncEmailWriterTest, TestWriteAttachments)
{
	fprintf(stderr, "Testing with one attachment\n");
	TestWriteEmail(1);
	fprintf(stderr, "Testing with two attachments\n");
	TestWriteEmail(2);
	fprintf(stderr, "Testing with three attachments\n");
	TestWriteEmail(3);
}

TEST(AsyncEmailWriterTest, TestMissingFile)
{
	Email email;
	InitEmail(email);
	
	fprintf(stderr, "Testing missing part file\n");
	
	MojRefCountedPtr<AsyncEmailWriter> writer( new AsyncEmailWriter(email) );

	// Create a body part
	EmailPartList partList;
	partList.push_back( CreateBodyPart("does-not-exist") );
	writer->SetPartList(partList);

	MojRefCountedPtr<ByteBufferOutputStream> bbos( new ByteBufferOutputStream() );
	writer->SetOutputStream(bbos);

	boost::shared_ptr<MockAsyncIOChannelFactory> ioFactory( new MockAsyncIOChannelFactory() );
	writer->SetAsyncIOChannelFactory(ioFactory);
	
	MojRefCountedPtr<AsyncEmailWriterResult> writeResult(new AsyncEmailWriterResult());
	writer->WriteEmail(writeResult->GetSlot());
	
	// Make sure it's done writing
	ASSERT_TRUE( writeResult->Done() );

	// Should have reported an exception
	ASSERT_TRUE( writeResult->GetException() );
}
