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

#include "email/BufferedEmailParser.h"
#include "data/Email.h"
#include "data/EmailAddress.h"
#include <gtest/gtest.h>
#include "client/MockBusClient.h"
#include "client/FileCacheClient.h"
#include "client/MockFileCacheService.h"
#include "email/MockAsyncIOChannel.h"

using namespace std;

void FeedParser(const MojRefCountedPtr<BufferedEmailParser>& parser, const std::string& text)
{
	for (size_t i = 0; i < text.size(); i++) {
		parser->FeedChar(text[i]);
	}
}

TEST(BufferedEmailParserTest, TestHeaders)
{
	MojRefCountedPtr<BufferedEmailParser> parser(new BufferedEmailParser());

	parser->EnableHeaderParsing();
	parser->Begin();
	EmailPtr email = parser->GetEmail();

	FeedParser(parser, "From: ");
	FeedParser(parser, "test@example.com\r\n");
	FeedParser(parser, "Subj");
	FeedParser(parser, "ect: Hello World\r\n");

	EXPECT_EQ( "test@example.com", email->GetFrom()->GetAddress() );

	FeedParser(parser, "\r\n");

	email = parser->GetEmail();
	EXPECT_EQ( "Hello World", email->GetSubject() );
}

TEST(BufferedEmailParserTest, TestBodies)
{
	MojRefCountedPtr<BufferedEmailParser> parser(new BufferedEmailParser());
	MockBusClient busClient;
	FileCacheClient fileCacheClient(busClient);

	// Setup filecache
	MockFileCacheService fcService;
	busClient.AddMockService(fcService);

	// Setup channel factory
	boost::shared_ptr<MockAsyncIOChannelFactory> factory(new MockAsyncIOChannelFactory());

	parser->SetChannelFactory(factory);
	parser->EnableHeaderParsing();
	parser->EnableBodyParsing(fileCacheClient, 0);
	parser->Begin();
	FeedParser(parser, "From: test@example.com\r\n");
	FeedParser(parser, "Subject: Test bodies\r\n");
	FeedParser(parser, "Content-type: multipart/alternative; boundary=BOUNDARY\r\n");
	FeedParser(parser, "\r\n");
	FeedParser(parser, "--BOUNDARY\r\n");
	FeedParser(parser, "Content-type: text/plain\r\n");
	FeedParser(parser, "\r\n");
	FeedParser(parser, "BODY A LINE 1\r\n");
	FeedParser(parser, "BODY A LINE 2\r\n");
	FeedParser(parser, "--BOUNDARY\r\n");
	FeedParser(parser, "Content-type: text/html\r\n");
	FeedParser(parser, "\r\n");
	FeedParser(parser, "BODY B LINE 1\r\n");
	FeedParser(parser, "BODY B LINE 2\r\n");
	FeedParser(parser, "--BOUNDARY--\r\n");
	parser->End();

	busClient.ProcessRequests();

	string dataA = factory->GetWrittenData(fcService.GetPathFor(0));
	EXPECT_EQ( "BODY A LINE 1\r\nBODY A LINE 2\r\n", dataA );

	string dataB = factory->GetWrittenData(fcService.GetPathFor(1));
	EXPECT_EQ( "BODY B LINE 1\r\nBODY B LINE 2\r\n", dataB );
}
