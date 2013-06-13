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

#include "sync/FolderListDiff.h"
#include <vector>
#include <gtest/gtest.h>

using namespace std;

boost::shared_ptr<ImapFolder> CreateFolder(const char* name)
{
	ImapFolderPtr folder(new ImapFolder());
	folder->SetFolderName(name);
	return folder;
}

TEST(FolderListDiffTest, TestEmpty)
{
	FolderListDiff diff;
	vector<ImapFolderPtr> local;
	vector<ImapFolderPtr> remote;
	
	diff.GenerateDiff(local, remote);
}

TEST(FolderListDiffTest, TestDiff)
{
	FolderListDiff diff;
	vector<ImapFolderPtr> local;
	vector<ImapFolderPtr> remote;
	
	local.push_back( CreateFolder("INBOX") );
	local.push_back( CreateFolder("Junk") );
	local.push_back( CreateFolder("LocalOnly") );
	
	remote.push_back( CreateFolder("INBOX") );
	remote.push_back( CreateFolder("Junk") );
	remote.push_back( CreateFolder("RemoteOnly") );

	diff.GenerateDiff(local, remote);
	ASSERT_EQ( (size_t) 1, diff.GetNewFolders().size() );
	ASSERT_EQ( (size_t) 1, diff.GetDeletedFolders().size() );
	
	EXPECT_EQ( "RemoteOnly", diff.GetNewFolders()[0]->GetFolderName() );
	EXPECT_EQ( "LocalOnly", diff.GetDeletedFolders()[0]->GetFolderName() );
}
