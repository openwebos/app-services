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

#include "data/EmailAddress.h"
#include "data/CommonData.h"
#include "email/MimeHeaderWriter.h"
#include <gtest/gtest.h>

TEST(MimeHeaderWriterTest, TestWriteHeader)
{
	std::string output;
	MimeHeaderWriter writer(output);
	
	writer.WriteHeader("Subject", "Test");
	
	EXPECT_EQ("Subject: Test\r\n", output);
}

TEST(MimeHeaderWriterTest, TestWriteEncodedHeader)
{
	std::string output;
	MimeHeaderWriter writer(output);

	writer.WriteHeader("Subject", "Test \xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", true);

	EXPECT_EQ("Subject: =?UTF-8?Q?Test_=E6=97=A5=E6=9C=AC=E8=AA=9E?=\r\n", output);
}

TEST(MimeHeaderWriterTest, TestWriteAddresses)
{
	std::string output;
	MimeHeaderWriter writer(output);
	
	EmailAddressListPtr list(new EmailAddressList());
	EmailAddressPtr addr1( new EmailAddress("jon.doe@example.com") );
	list->push_back(addr1);
	
	writer.WriteAddressHeader("To", list);
	EXPECT_EQ("To: <jon.doe@example.com>\r\n", output);
	
	output.clear();
	
	EmailAddressPtr addr2( new EmailAddress("Jane Doe", "jane.doe@example.com") );
	list->push_back(addr2);
	
	writer.WriteAddressHeader("To", list);
	
	EXPECT_EQ("To: <jon.doe@example.com>, \"Jane Doe\" <jane.doe@example.com>\r\n", output);
}
