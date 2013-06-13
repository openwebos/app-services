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

#include "stream/DeflaterOutputStream.h"
#include "stream/InflaterInputStream.h"
#include "stream/MockStreams.h"
#include "util/Hexdump.h"
#include <gtest/gtest.h>

using namespace std;

void Roundtrip(const std::string& text)
{
	MojRefCountedPtr<MockOutputStream> mos(new MockOutputStream());
	MojRefCountedPtr<DeflaterOutputStream> dos(new DeflaterOutputStream(mos));

	dos->Write(text.data(), text.length());
	dos->Flush();

	string deflated = mos->GetBuffer();

	MojRefCountedPtr<MockInputStream> mis(new MockInputStream());
	MojRefCountedPtr<InflaterInputStream> iis(new InflaterInputStream(mis));
	MojRefCountedPtr<MockInputStreamReader> reader(new MockInputStreamReader(iis));

	mis->Feed(deflated);
	mis->FlushBuffer();

	string inflated = reader->GetBuffer();

	if(text != inflated) {
		string original = Hexdump::FormatWrapped(inflated.data(), inflated.length(), 32);
		string roundtripped = Hexdump::FormatWrapped(inflated.data(), inflated.length(), 32);
		FAIL() << ("original text [" + original + "] != deflated+inflated [" + roundtripped + "]");
	}

	ASSERT_EQ( text, inflated );
}

TEST(DeflateInflateTest, TestDeflateInflate)
{
	Roundtrip("4 select \"INBOX.Trash\"\r\n");

	// Big string
	string big;
	for(int i = 0; i < 1000; i++) {
		big += "ABCDEFGHIJKMLOPQRSTUVWXYZ0123456789";
	}

	Roundtrip(big);
}
