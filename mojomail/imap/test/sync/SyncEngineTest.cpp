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

#include "sync/SyncEngine.h"
#include "ImapPrivate.h"
#include <gtest/gtest.h>

SyncEngine::EmailStub MakeStub(UID uid, const char* idStr)
{
	MojString id;
	id.assign(idStr);
	return SyncEngine::EmailStub(uid, id);
}

class TestSyncEngine : public SyncEngine
{
public:
	TestSyncEngine(const vector<UID>& remoteList) : SyncEngine(remoteList, s_emptyList, s_emptyList, s_emptyList, s_emptyList) {}
	virtual ~TestSyncEngine() {}

	static vector<UID> s_emptyList;
};
vector<UID> TestSyncEngine::s_emptyList;

void Expect(const SyncEngine& engine, size_t expectNew, size_t expectDeleted)
{
	EXPECT_EQ( expectNew, engine.GetNewUIDs().size() );
	EXPECT_EQ( expectDeleted, engine.GetDeletedIds().size() );
}

TEST(SyncEngineTest, TestAddDelete)
{
	vector<SyncEngine::EmailStub> local;
	vector<UID> remote;

	local.push_back(MakeStub(10, "id10"));

	// one local entry, not on server
	{
		TestSyncEngine engine(remote);
		engine.Diff(local, false);
		Expect(engine, 0, 1); // 1 deleted
	}

	remote.push_back(10);

	// one existing email on both local and server
	{
		TestSyncEngine engine(remote);
		engine.Diff(local, false);
		Expect(engine, 0, 0); // no change
	}

	remote.push_back(20);
	remote.push_back(30);

	// two new emails on server, one existing
	{
		TestSyncEngine engine(remote);
		engine.Diff(local, false);
		Expect(engine, 2, 0);
	}

	local.push_back(MakeStub(25, "id25"));

	// two new emails, one existing, one not on server in between
	{
		TestSyncEngine engine(remote);
		engine.Diff(local, false);
		Expect(engine, 2, 1);
	}
}

TEST(SyncEngineTest, TestBatch)
{
	vector<SyncEngine::EmailStub> batch1, batch2;
	vector<UID> remote;

	for(int i = 10; i < 100; i += 10) {
		remote.push_back(i);
	}

	TestSyncEngine engine(remote);

	batch1.push_back(MakeStub(5, "id5"));
	batch1.push_back(MakeStub(15, "id15"));
	batch1.push_back(MakeStub(20, "id20"));

	engine.Diff(batch1, true);
	// Local:  5  N 15 20 ...
	// Remote: D 10  D 20 30
	Expect(engine, 1, 2);

	engine.Clear();

	batch2.push_back(MakeStub(40, "id45"));
	batch2.push_back(MakeStub(45, "id45"));
	batch2.push_back(MakeStub(50, "id50"));
	batch2.push_back(MakeStub(65, "id65"));

	engine.Diff(batch2, false);
	// Local:   N 40 45 50  N 65  N  N  N
	// Remote: 30 40  D 50 60  D 70 80 90
	Expect(engine, 5, 2);
}

TEST(SyncEngineTest, TestDeletedFlag)
{
	vector<SyncEngine::EmailStub> local;
	vector<UID> remote;
	vector<UID> deleted;
	vector<UID> empty;

	remote.push_back(10); deleted.push_back(10); local.push_back(MakeStub(10, "id10"));
	remote.push_back(20); deleted.push_back(20);

	SyncEngine engine(remote, deleted, empty, empty, empty);
	engine.Diff(local, true);
	Expect(engine, 0, 1); // one existing deleted, one never-downloaded deleted
}
