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

#include "stream/ByteBufferOutputStream.h"
#include <gtest/gtest.h>
#include <string>

using namespace std;

TEST(ByteBufferOutputStreamTest, TestBasic)
{
	int BUF_SIZE = 100;
	char buf[BUF_SIZE];
	int nread;
	
	ByteBufferOutputStream bbos;
	
	// Should be initially empty
	nread = bbos.ReadFromBuffer(buf, 10);
	ASSERT_EQ( 0, (int) nread );
	
	// Write and read
	strncpy(buf, "XXX@1", BUF_SIZE);
	bbos.Write("xyz", 3);
	nread = bbos.ReadFromBuffer(buf, 3);
	ASSERT_EQ( 3, (int) nread );
	ASSERT_EQ("xyz@1", string(buf, 5));
	
	// Should be empty again
	nread = bbos.ReadFromBuffer(buf, 10);
	ASSERT_EQ( 0, (int) nread );
	
	// Write and read some of it
	strncpy(buf, "XXXXX@2", BUF_SIZE);
	bbos.Write("hello world", 11);
	nread = bbos.ReadFromBuffer(buf, 5);
	ASSERT_EQ( 5, (int) nread );
	ASSERT_EQ("hello@2", string(buf, 7));
	
	// Read the rest
	strncpy(buf, "XXXXXX@3", BUF_SIZE);
	nread = bbos.ReadFromBuffer(buf, 100);
	ASSERT_EQ( 6, (int) nread );
	ASSERT_EQ(" world@3", string(buf, 8));
	
	// Should be empty again
	nread = bbos.ReadFromBuffer(buf, 10);
	ASSERT_EQ( 0, (int) nread);
}
