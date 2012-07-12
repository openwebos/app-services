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

#include "mimeparser/EmailHeaderFieldParser.h"
#include "data/EmailAddress.h"
#include "data/EmailPart.h"
#include <gtest/gtest.h>

using namespace std;

EmailAddressList ParseAddressField(const std::string& fieldValue)
{
	EmailHeaderFieldParser parser;
	EmailAddressList addressList;
	parser.ParseAddressListField(fieldValue, addressList);
	return addressList;
}

void DumpList(EmailAddressList& list)
{
	for (size_t i = 0; i < list.size(); i++) {
		EmailAddressPtr addr = list.at(i);
		cerr << i << ": " << "\"" << addr->GetDisplayName() << "\"" << " <" << addr->GetAddress() << ">" << endl;
	}
}

void CheckAddress(EmailAddressList& list, int index, const std::string& displayName, const std::string& address)
{
	EmailAddressPtr emailAddress = list.at(index);
	EXPECT_EQ( displayName, emailAddress->GetDisplayName() );
	EXPECT_EQ( address, emailAddress->GetAddress() );
}

TEST (EmailHeaderFieldParserTest, TestEmpty)
{
	EmailAddressList list;

	list = ParseAddressField("");
	ASSERT_EQ(0u, list.size());

	list = ParseAddressField("  ");
	ASSERT_EQ(0u, list.size());
}

TEST (EmailHeaderFieldParserTest, TestParseSimpleAddress)
{
	EmailAddressList list;

	list = ParseAddressField("test@example.com");
	ASSERT_EQ(1u, list.size());
	CheckAddress(list, 0, "", "test@example.com");
}

TEST (EmailHeaderFieldParserTest, TestParseNameAndAddress)
{
	EmailAddressList list;

	list = ParseAddressField("Test Tester 1 <test@example.com>");
	EXPECT_EQ(1u, list.size());
	CheckAddress(list, 0, "Test Tester 1", "test@example.com");

	list = ParseAddressField("\"Test Tester 2\" <test@example.com>");
	EXPECT_EQ(1u, list.size());
	CheckAddress(list, 0, "Test Tester 2", "test@example.com");

	list = ParseAddressField("Test Tester 3 < test @ example.com >");
	EXPECT_EQ(1u, list.size());
	CheckAddress(list, 0, "Test Tester 3", "test@example.com");

	list = ParseAddressField("<test@example.com>");
	EXPECT_EQ(1u, list.size());
	CheckAddress(list, 0, "", "test@example.com");
}

TEST (EmailHeaderFieldParserTest, TestParseList)
{
	EmailAddressList list;

	list = ParseAddressField("Test Tester <test@example.com>, <foo@bar.com>, nobody@nowhere.com");
	EXPECT_EQ(3u, list.size());
	CheckAddress(list, 0, "Test Tester", "test@example.com");
	CheckAddress(list, 1, "", "foo@bar.com");
	CheckAddress(list, 2, "", "nobody@nowhere.com");
}

TEST (EmailHeaderFieldParserTest, TestParseGroup)
{
	EmailAddressList list;

	list = ParseAddressField("my group: test@example.com, Foo <foo@bar.com>;");
	ASSERT_EQ(2u, list.size());
	CheckAddress(list, 0, "", "test@example.com");
	CheckAddress(list, 1, "Foo", "foo@bar.com");
}

TEST (EmailHeaderFieldParserTest, TestRfc2047Address)
{
	EmailAddressList list;

	list = ParseAddressField("=?iso-8859-1?q?One?= =?utf-8?q?Word?= <foo@bar.com>");
	ASSERT_EQ(1u, list.size());
	CheckAddress(list, 0, "OneWord", "foo@bar.com");
}

TEST (EmailHeaderFieldParserTest, TestParseEmptyGroup)
{
	EmailAddressList list;

	list = ParseAddressField("undisclosed recipients: ;");
	ASSERT_EQ(0u, list.size());
}

TEST (EmailHeaderFieldParserTest, TestContentId)
{
	EmailPart part(EmailPart::INLINE);

	EmailHeaderFieldParser parser;

	parser.ParsePartHeaderField(part, "content-id", "<CONTENTID>");

	ASSERT_EQ("CONTENTID", part.GetContentId());
}
