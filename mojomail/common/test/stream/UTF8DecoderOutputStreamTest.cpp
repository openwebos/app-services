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

#include "stream/UTF8DecoderOutputStream.h"
#include "stream/ByteBufferOutputStream.h"
#include <gtest/gtest.h>

TEST(UTF8DecoderOutputStreamTest, TestUTF8Decoder)
{
	int BUF_SIZE = 1024;
	char buf[BUF_SIZE];
	int nread;

	const char source[] = { 0xE0, 0xE8, 0xEC, 0xF2, 0xF9, 0x0000 };  //"à, è, ì, ò, ù," in ISO-8859-1;

	int32_t srcCount=sizeof(source);

	MojRefCountedPtr<ByteBufferOutputStream> bbos( new ByteBufferOutputStream() );
	UTF8DecoderOutputStream utf8dos(bbos, "ISO-8859-1");

	utf8dos.Write(source, srcCount);
	utf8dos.Flush();
	nread = bbos->ReadFromBuffer(buf, BUF_SIZE);
	ASSERT_EQ(11, (int) nread);

	const char control[] = {0xC3, 0xA0, 0xC3, 0xA8, 0xC3, 0xAC, 0xC3, 0xB2, 0xC3, 0xB9, 0x0000};  //"à, è, ì, ò, ù," in UTF8

	ASSERT_STREQ(control, std::string(buf, 11).c_str());
}
