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

#include "stream/MockStreams.h"
#include "stream/LineReader.h"
#include "MockDoneSlot.h"
#include <gtest/gtest.h>

using namespace std;


class LineReaderDoneSlot : public MockDoneSlot
{
public:
	LineReaderDoneSlot(const LineReaderPtr& lineReader) : m_lineReader(lineReader) {}
protected:
	LineReaderPtr m_lineReader;
};

TEST(LineReaderTest, TestBlanks)
{
	MojRefCountedPtr<MockInputStream> in(new MockInputStream());
	LineReaderPtr lineReader(new LineReader(in));

	// Need a special done slot because ReadLine is only valid from within the callback
	class ReadBlanksSlot : public LineReaderDoneSlot
	{
	public:
		ReadBlanksSlot(const LineReaderPtr& lineReader) : LineReaderDoneSlot(lineReader) {}

		void Done()
		{
			ASSERT_TRUE( m_lineReader->MoreLinesInBuffer() );
			string line1 = m_lineReader->ReadLine(true);
			ASSERT_EQ( "Hello\r\n", line1 );

			ASSERT_TRUE( m_lineReader->MoreLinesInBuffer() );
			string line2 = m_lineReader->ReadLine(true);
			ASSERT_EQ( "\r\n", line2 );

			ASSERT_TRUE( m_lineReader->MoreLinesInBuffer() );
			string line3 = m_lineReader->ReadLine(false); // don't include newline in string
			ASSERT_EQ( "", line3 );

			ASSERT_TRUE( m_lineReader->MoreLinesInBuffer() );
			string line4 = m_lineReader->ReadLine(true);
			ASSERT_EQ( "World\r\n", line4 );
		}
	};

	ReadBlanksSlot readBlanksSlot(lineReader);

	lineReader->WaitForLine(readBlanksSlot.GetSlot());

	in->Feed("Hello\r\n" "\r\n" "\r\n" "World\r\n");
}

TEST(LineReaderTest, TestNewlineAuto)
{
	MojRefCountedPtr<MockInputStream> in(new MockInputStream());
	LineReaderPtr lineReader(new LineReader(in));

	// Need a special done slot because ReadLine is only valid from within the callback
	class TestNewlinesSlot : public LineReaderDoneSlot
	{
	public:
		TestNewlinesSlot(const LineReaderPtr& lineReader) : LineReaderDoneSlot(lineReader) {}

		void Done()
		{
			ASSERT_EQ( m_lineReader->ReadLine(false), "Hello world" );
			ASSERT_EQ( m_lineReader->ReadLine(false), "Test 1" );
			ASSERT_EQ( m_lineReader->ReadLine(false), "" );
			ASSERT_EQ( m_lineReader->ReadLine(false), "Test 2" );

			ASSERT_TRUE( !m_lineReader->MoreLinesInBuffer() );
		}
	};

	TestNewlinesSlot testSlot(lineReader);

	lineReader->SetNewlineMode(LineReader::NewlineMode_Auto);
	lineReader->WaitForLine(testSlot.GetSlot());

	in->Feed("Hello world\nTest 1\r\n\rTest 2\r\n");
}
