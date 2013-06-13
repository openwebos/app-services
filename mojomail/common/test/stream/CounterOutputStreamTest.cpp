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

#include "stream/CounterOutputStream.h"
#include <gtest/gtest.h>

TEST(CounterOutputStreamTest, TestCounter)
{
	CounterOutputStream counter;
	
	ASSERT_EQ(0, (int) counter.GetBytesWritten());
	
	counter.Write("test", 4);
	ASSERT_EQ(4, (int) counter.GetBytesWritten());
	
	counter.Write("hello world", 11);
	ASSERT_EQ(15, (int) counter.GetBytesWritten());
	
	counter.Write("", 0);
	ASSERT_EQ(15, (int) counter.GetBytesWritten());
	
	counter.Write("hello world", 5);
	ASSERT_EQ(20, (int) counter.GetBytesWritten());
}
