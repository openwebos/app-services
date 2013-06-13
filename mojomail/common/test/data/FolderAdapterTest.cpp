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

#include "data/FolderAdapter.h"
#include "data/Folder.h"
#include "TestUtils.h"
#include <gtest/gtest.h>

TEST(FolderAdapterTest, TestRoundtrip)
{
	Folder folder;
	MojObject input;
	std::string test_data = QUOTE_JSON((
		{"_kind": "com.palm.folder:1",
		 "accountId": "A+1",
		 "parentId": "A+2",
		 "displayName": "test",
		 "favorite": true
		}
	));
	
	input.fromJson(test_data.c_str());
	FolderAdapter::ParseDatabaseObject(input, folder);
	
	MojObject output;
	FolderAdapter::SerializeToDatabaseObject(folder, output);
	
	ASSERT_EQ(input, output);
}

TEST(FolderAdapterTest, TestRoundtripId)
{
	Folder folder;
	MojObject input;
	std::string test_data = QUOTE_JSON((
		{"_kind": "com.palm.folder:1",
		 "_id": "A+0",
		 "accountId": "A+1",
		 "parentId": "A+2",
		 "displayName": "test",
		 "favorite": true
		}
	));

	input.fromJson(test_data.c_str());
	FolderAdapter::ParseDatabaseObject(input, folder);

	MojObject output;
	FolderAdapter::SerializeToDatabaseObject(folder, output);

	ASSERT_EQ(input, output);
}
