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

#include "protocol/ListResponseParser.h"
#include "client/MockImapSession.h"
#include "data/ImapFolder.h"
#include "protocol/MockDoneSlot.h"
#include "MockTestSetup.h"
#include <gtest/gtest.h>

using namespace std;

class MockListResponseParser : public ListResponseParser
{
public:
	MockListResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
	: ListResponseParser(session, doneSlot)
	{}
	
	void ParseFolder(const string& line, ImapFolder& folder)
	{
		ListResponseParser::ParseFolder(line, folder);
	}
};

TEST(ListResponseParserTest, TestParseFolder)
{
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	MockDoneSlot doneSlot;
	MojRefCountedPtr<MockListResponseParser> command(new MockListResponseParser(session, doneSlot.GetSlot()));
	
	if(true) {
		ImapFolder folder;
		command->ParseFolder("LIST () \"/\" \"INBOX\"", folder);
		EXPECT_EQ( "/", folder.GetDelimiter() );
		EXPECT_EQ( "INBOX", folder.GetFolderName() );
		EXPECT_EQ( "INBOX", folder.GetDisplayName() );
	}
	
	if(true) {
		ImapFolder folder;
		command->ParseFolder("LIST () \"/\" \"INBOX/Subfolder\"", folder);
		EXPECT_EQ( "/", folder.GetDelimiter() );
		EXPECT_EQ( "INBOX/Subfolder", folder.GetFolderName() );
		EXPECT_EQ( "Subfolder", folder.GetDisplayName() );
	}

	if(true) {
		ImapFolder folder;
		command->ParseFolder("LIST () \"/\" \"[Gmail]/&V4NXPpCuTvY-\"", folder);
		EXPECT_EQ( "/", folder.GetDelimiter() );
		EXPECT_EQ( "[Gmail]/&V4NXPpCuTvY-", folder.GetFolderName() );
		//fprintf(stderr, "Decoded: [%s]\n", folder.GetDisplayName().c_str());
		EXPECT_EQ( "\xe5\x9e\x83\xe5\x9c\xbe\xe9\x82\xae\xe4\xbb\xb6", folder.GetDisplayName() );
	}
}

TEST(ListResponseParserTest, TestUnquoted)
{
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	MockDoneSlot doneSlot;
	MojRefCountedPtr<MockListResponseParser> command(new MockListResponseParser(session, doneSlot.GetSlot()));

	ImapFolder folder;

	command->ParseFolder("LIST (\\HasNoChildren) \"\\\\\" folder01", folder);

	EXPECT_EQ( "folder01", folder.GetFolderName() );
}
