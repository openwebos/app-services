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

#include "stream/QuotePrintableDecoderOutputStream.h"
#include "stream/ByteBufferOutputStream.h"
#include <gtest/gtest.h>

TEST(QuotePrintableDecoderOutputStreamTest, TestQuotePrintableDecoder)
{
	int BUF_SIZE = 100;
	char buf[BUF_SIZE];
	int nread;

	MojRefCountedPtr<ByteBufferOutputStream> bbos( new ByteBufferOutputStream() );
	QuotePrintableDecoderOutputStream qpdos(bbos);

	qpdos.Write("If you believe that truth=3Dbeauty, then surely=20=\r\nmathematics is the most beautiful branch of philosophy.", 108);
	nread = bbos->ReadFromBuffer(buf, 1024);
	ASSERT_EQ(101, (int) nread);
	ASSERT_EQ("If you believe that truth=beauty, then surely mathematics is the most beautiful branch of philosophy.", std::string(buf, 101));
}
