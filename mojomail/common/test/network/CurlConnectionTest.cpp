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

#include "network/CurlConnection.h"
#include <gtest/gtest.h>

using namespace std;

// Get around protected method restrictions
class TestCurlConnection : public CurlConnection
{
public:
	static string ExtractHostnameFromUrl(const string& url)
	{
		return CurlConnection::ExtractHostnameFromUrl(url);
	}
};

string ExtractHostnameFromUrl(const string& url)
{
	return TestCurlConnection::ExtractHostnameFromUrl(url);
}

TEST(CurlConnectionTest, TestExtractHostname)
{
	EXPECT_EQ( "us-owa.palm.com", ExtractHostnameFromUrl("https://us-owa.palm.com") );

	EXPECT_EQ( "us-owa.palm.com", ExtractHostnameFromUrl("https://us-owa.palm.com/") );

	EXPECT_EQ( "example.com", ExtractHostnameFromUrl("https://example.com/servlet/traveler") );

	EXPECT_EQ( "example.com", ExtractHostnameFromUrl("http://example.com:1234") );

	EXPECT_EQ( "123.123.0.1", ExtractHostnameFromUrl("http://123.123.0.1:1234/") );

	EXPECT_EQ( "2001:0db8:85a3:0000:0000:8a2e:0370:7334", ExtractHostnameFromUrl("https://[2001:0db8:85a3:0000:0000:8a2e:0370:7334]") );

	EXPECT_EQ( "2001:0db8:85a3:0000:0000:8a2e:0370:7334", ExtractHostnameFromUrl("https://[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:1234/servlet/traveler") );
}
