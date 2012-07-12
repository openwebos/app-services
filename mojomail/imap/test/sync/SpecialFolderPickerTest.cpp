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

#include "sync/SpecialFolderPicker.h"
#include "data/ImapFolder.h"
#include "ImapPrivate.h"
#include <vector>
#include <gtest/gtest.h>

// Create a folder using / as the delimiter
ImapFolderPtr CreateFolder(string name)
{
	ImapFolderPtr folder = make_shared<ImapFolder>();
	
	size_t slash = name.find_first_of("/");
	if(slash == string::npos)
		folder->SetDisplayName(name);
	else
		folder->SetDisplayName(name.substr(slash+1));
	folder->SetFolderName(name);
	folder->SetDelimiter("/");
	return folder;
}

TEST(SpecialFolderPickerTest, TestSpecialFolders)
{
	vector<ImapFolderPtr> folders;
	ImapFolderPtr inbox, trash, sent, drafts;
	
	folders.push_back( inbox = CreateFolder("INBOX") );
	folders.push_back( trash = CreateFolder("Deleted messages") );
	folders.push_back( sent = CreateFolder("Sent Stuff") );
	folders.push_back( drafts = CreateFolder("Drafts") );
	
	SpecialFolderPicker picker(folders);
	EXPECT_EQ( inbox, picker.MatchInbox() );
	EXPECT_EQ( trash, picker.MatchTrash() );
	EXPECT_EQ( sent, picker.MatchSent() );
	EXPECT_EQ( drafts, picker.MatchDrafts() );
}

// Test multiple folders matching one use
// e.g. one folder called "Trash" and one called "Deleted Items"
TEST(SpecialFolderPickerTest, TestMultipleMatches)
{
	vector<ImapFolderPtr> folders;
	ImapFolderPtr deletedMessages, trash;
	
	folders.push_back( trash = CreateFolder("Trash") );
	folders.push_back( deletedMessages = CreateFolder("Deleted messages") );
	
	SpecialFolderPicker picker(folders);
	EXPECT_EQ( deletedMessages, picker.MatchTrash() );
}

