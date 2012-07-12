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

#include "protocol/UidSearchResponseParser.h"
#include <gtest/gtest.h>

#include <sstream>

TEST(UidSearchResponseParserTest, ParseUids)
{
	std::vector<unsigned int> uids;
	
	uids.clear();
	EXPECT_TRUE( UidSearchResponseParser::ParseUids("", uids) );
	EXPECT_TRUE( uids.empty() );
	
	uids.clear();
	EXPECT_TRUE( UidSearchResponseParser::ParseUids(" ", uids) );
	EXPECT_TRUE( uids.empty() );
	
	uids.clear();
	EXPECT_TRUE( UidSearchResponseParser::ParseUids("123", uids) );
	EXPECT_EQ( (size_t) 1, uids.size() );
	EXPECT_EQ( UID(123), uids[0] );
}

TEST(UidSearchResponseParserTest, TestStress)
{
	std::stringstream ss;
	for(unsigned int i = 1; i < 10000; ++i) {
		ss << i << "  ";
	}
	std::string s = ss.str();
	
	std::vector<unsigned int> uids;
	UidSearchResponseParser::ParseUids(s, uids);
	
	ASSERT_EQ( (size_t) 9999, uids.size() );
	for(unsigned int i = 1; i < 10000; ++i) {
		EXPECT_EQ( i, uids.at(i-1) );
	}
}
