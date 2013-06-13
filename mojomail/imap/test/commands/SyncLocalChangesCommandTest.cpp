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

#include "client/MockImapSession.h"
#include "client/MockImapClient.h"
#include "commands/SyncLocalChangesCommand.h"
#include "TestUtils.h"
#include "MockTestSetup.h"
#include <gtest/gtest.h>

class TestMockDatabase : public MockDatabase
{
public:
	virtual void MergeFlags(Signal::SlotRef slot, const MojObject::ObjectVec& array)
	{
		m_lastMerged = array;
		Default(__func__, slot);
	}

	MojObject::ObjectVec	m_lastMerged;
};

TEST(SyncLocalChangesCommandTest, TestSyncChanges)
{
	MockTestSetup setup;
	TestMockDatabase& db = setup.GetTestDatabase<TestMockDatabase>();
	MockImapSession& session = setup.GetSession();

	MojObject response;

	const MockInputStreamPtr& is = session.GetMockInputStream();
	const MockOutputStreamPtr& os = session.GetMockOutputStream();

	boost::shared_ptr<ImapAccount> account(new ImapAccount());

	setup.GetClient().SetAccount(account);

	MojString folderId;
	folderId.assign("TEST+FOLDERID");

	MojRefCountedPtr<SyncLocalChangesCommand> command(new SyncLocalChangesCommand(session, folderId));

	response = QUOTE_JSON_OBJ((
		{"results":
		[
		 {"_id": "id1", "_rev": 1, "uid": 10, "flags": {"read": true}, "lastSyncFlags": {"read": true}},
		 {"_id": "id2", "_rev": 2, "uid": 20, "flags": {"read": true}, "lastSyncFlags": {"read": false}},
		 {"_id": "id3", "_rev": 3, "uid": 30, "flags": {"read": false}, "lastSyncFlags": {"read": true}},
		]}
	));
	db.SetResponse("GetEmailChanges", response);

	response = QUOTE_JSON_OBJ((
			{"results":
			[
			 {"_id": "id1", "_rev": 4, "uid": 10, "destFolderId": "1oZ"},
			]}
		));
	db.SetResponse("GetMovedEmails", response);

	response = QUOTE_JSON_OBJ((
			{"results":
			[
			 {"serverFolderName": "Saved"},
			]}
		));
	db.SetResponse("GetFolderName", response);

	response = QUOTE_JSON_OBJ((
				{"results":[]}
			));
	db.SetResponse("DeleteEmailIds", response);

	command->Run();

	// UID 20 should be marked read
	ASSERT_EQ( "~A1 UID STORE 20 +FLAGS.SILENT (\\Seen)", os->GetLine() );

	// UID 30 should be marked unread
	ASSERT_EQ( "~A2 UID STORE 30 -FLAGS.SILENT (\\Seen)", os->GetLine() );

	is->FeedLine("~A1 OK");

	response = QUOTE_JSON_OBJ((
		{"results":[{"id": "id2", "rev": 200}, {"id": "id3", "rev": 300}]}
	));
	db.SetResponse("MergeFlags", response);


	response = QUOTE_JSON_OBJ((
				{"results":[]}
			));
		db.SetResponse("GetDeletedEmails", response);

	is->FeedLine("~A2 OK");

	// UID 30 should be marked unread
	ASSERT_EQ( "~A3 UID COPY 10 \"Saved\"", os->GetLine() );
	is->FeedLine("~A3 OK");

	ASSERT_EQ("~A4 UID STORE 10 +FLAGS.SILENT (\\Deleted)", os->GetLine());
	is->FeedLine("~A4 OK");

	ASSERT_EQ("~A5 EXPUNGE", os->GetLine());
	is->FeedLine("~A5 OK");


	// TODO check merged
}
