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

#include <gtest/gtest.h>
#include "protocol/ExamineResponseParser.h"
#include "protocol/MockDoneSlot.h"
#include "test/MockTestSetup.h"

/*
 * FLAGS (\Answered \Flagged \Draft \Deleted \Seen $Forwarded SEEN)
 * OK [PERMANENTFLAGS (\Answered \Flagged \Draft \Deleted \Seen $Forwarded SEEN \*)] Flags permitted.
 * OK [UIDVALIDITY 614856550] UIDs valid.
 * 28 EXISTS
 * 0 RECENT
 * OK [UIDNEXT 859] Predicted next UID.
 2 OK [READ-WRITE] INBOX selected. (Success)
 */

TEST(ExamineResponseParserTest, TestExamineResponse)
{
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	MockDoneSlot doneSlot;
	MojRefCountedPtr<ExamineResponseParser> parser(new ExamineResponseParser(session, doneSlot.GetSlot()));

	parser->HandleUntaggedResponse("FLAGS (\\Answered \\Flagged \\Draft \\Deleted \\Seen $Forwarded SEEN)");
	parser->HandleUntaggedResponse("OK [UIDVALIDITY 614856550] UIDs valid.");
	parser->HandleUntaggedResponse("28 EXISTS");
	parser->HandleUntaggedResponse("0 RECENT");
	parser->HandleUntaggedResponse("OK [UIDNEXT 859] Predicted next UID.");

	EXPECT_EQ( 28, parser->GetExistsCount() );
	EXPECT_EQ( 614856550u, parser->GetUIDValidity() );
}

TEST(ExamineResponseParserTest, TestUIDValidityOverflow)
{
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	MockDoneSlot doneSlot;
	MojRefCountedPtr<ExamineResponseParser> parser(new ExamineResponseParser(session, doneSlot.GetSlot()));

	// 64-bit uid validity will overflow
	parser->HandleUntaggedResponse("OK [UIDVALIDITY 34359738468] UIDs valid.");

	EXPECT_EQ( 100u, parser->GetUIDValidity() );
}
