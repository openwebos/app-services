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

#include <stream/CRLFTerminatedOutputStream.h>
#include <stream/ByteBufferOutputStream.h>
#include <stream/BaseOutputStream.h>
#include <gtest/gtest.h>
#include <string>

using namespace std;

TEST(CRLFTerminatedOutputStreamTest, TestNoNewline)
{
	MojRefCountedPtr<ByteBufferOutputStream> bbos(new ByteBufferOutputStream());
	MojRefCountedPtr<CRLFTerminatedOutputStream> crlfOS(new CRLFTerminatedOutputStream(bbos));

	string s("hello world");
	crlfOS->Write(s.data(), s.length());
	crlfOS->Flush();

	ASSERT_EQ("hello world\r\n", bbos->GetBuffer());
}

TEST(CRLFTerminatedOutputStreamTest, TestHasNewline)
{
	MojRefCountedPtr<ByteBufferOutputStream> bbos(new ByteBufferOutputStream());
	MojRefCountedPtr<CRLFTerminatedOutputStream> crlfOS(new CRLFTerminatedOutputStream(bbos));

	string s("hello world\r\n");
	crlfOS->Write(s.data(), s.length());
	crlfOS->Flush();

	ASSERT_EQ("hello world\r\n", bbos->GetBuffer());
}
