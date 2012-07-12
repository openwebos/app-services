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

#include "client/ImapRequestManager.h"
#include "client/ImapSession.h"
#include "core/MojRefCount.h"
#include "client/MockImapSession.h"
#include "MockTestSetup.h"
#include <gtest/gtest.h>

using namespace std;

class MockImapResponseParser : public ImapResponseParser
{
public:
	MockImapResponseParser(ImapSession& session)
	: ImapResponseParser(session), m_lastStatus(STATUS_UNKNOWN) {}
	
	void RunImpl()
	{
	}
	
	bool HandleUntaggedResponse(const string& line)
	{
		m_lastUntaggedResponseLine = line;
		return false;
	}
	
	void HandleResponse(ImapStatusCode status, const string& line)
	{
		m_lastStatus = status;
		m_lastResponseLine = line;
	}
	
	void ResetTest()
	{
		m_lastUntaggedResponseLine.clear();
		m_lastStatus = (ImapStatusCode) -1;
		m_lastResponseLine.clear();
	}
	
	string			m_lastUntaggedResponseLine;
	ImapStatusCode	m_lastStatus;
	string			m_lastResponseLine;
};

TEST(ImapRequestManagerTest, TestSplitOnce)
{
	string first;
	string rest;
	
	ImapResponseParser::SplitOnce("hello world", first, rest);
	EXPECT_EQ( "hello", first );
	EXPECT_EQ( "world", rest );
	
	ImapResponseParser::SplitOnce("allo   monde", first, rest);
	EXPECT_EQ( "allo", first );
	EXPECT_EQ( "monde", rest );
	
	ImapResponseParser::SplitOnce("word", first, rest);
	EXPECT_EQ( "word", first );
	EXPECT_EQ( "", rest );
	
	ImapResponseParser::SplitOnce("trailing   ", first, rest);
	EXPECT_EQ( "trailing", first );
	EXPECT_EQ( "", rest );
	
	ImapResponseParser::SplitOnce("", first, rest);
	EXPECT_EQ( "", first );
	EXPECT_EQ( "", rest );
}

TEST(ImapRequestManagerTest, TestSplitResponse)
{
	string tag;
	ImapStatusCode status;
	string rest;
	
	ImapResponseParser::SplitResponse("~A1 OK", tag, status, rest);
	EXPECT_EQ( "~A1", tag );
	EXPECT_TRUE( status == OK );
	EXPECT_EQ( "", rest );
	
	ImapResponseParser::SplitResponse("~A2 OK success", tag, status, rest);
	EXPECT_EQ( "~A2", tag );
	EXPECT_TRUE( status == OK );
	EXPECT_EQ( "success", rest );
	
	ImapResponseParser::SplitResponse("~A3 NO oh no", tag, status, rest);
	EXPECT_EQ( "~A3", tag );
	EXPECT_TRUE( status == NO );
	EXPECT_EQ( "oh no", rest );
	
	ImapResponseParser::SplitResponse("~A4 BAD invalid", tag, status, rest);
	EXPECT_EQ( "~A4", tag );
	EXPECT_TRUE( status == BAD );
	EXPECT_EQ( "invalid", rest );
}

#include <iostream>

TEST(ImapRequestManagerTest, TestHandleResponseLine)
{
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();
	
	MojRefCountedPtr<MockImapResponseParser> parser(new MockImapResponseParser(session));
	MojRefCountedPtr<ImapRequestManager> requestManager(new ImapRequestManager(session));

	const MockInputStreamPtr& is = session.GetMockInputStream();

	parser->ResetTest();
	requestManager->SendRequest("NOOP", parser, 0, true);
	is->FeedLine("* untagged");
	EXPECT_EQ( "untagged", parser->m_lastUntaggedResponseLine );
	
	is->FeedLine("~A1 OK");
	EXPECT_TRUE( parser->m_lastStatus == OK);
	EXPECT_EQ( "", parser->m_lastResponseLine );
	
	parser->ResetTest();
	requestManager->SendRequest("NOOP", parser, 0, true);
	is->FeedLine("~A2 NO oh no");
	EXPECT_TRUE( parser->m_lastStatus == NO);
	EXPECT_EQ( "oh no", parser->m_lastResponseLine );
}
