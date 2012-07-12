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

#include "util/StringUtils.h"
#include "util/Hexdump.h"
#include <gtest/gtest.h>

using namespace std;

TEST(StringUtilsTest, TestSanitizeASCII)
{
	string text;

	text = "Te\x96s\x96t";
	StringUtils::SanitizeASCII(text, "?");
	ASSERT_EQ( text, "Te?s?t" );
}

TEST(StringUtilsTest, TestSanitizeUTF8)
{
	string text;

	// Empty string
	text = "";
	StringUtils::SanitizeUTF8(text);
	ASSERT_EQ( "", text );

	// Bad character
	text = "\x95Te\x96st\x97";
	StringUtils::SanitizeUTF8(text);
	ASSERT_EQ( text, "\xef\xbf\xbdTe\xef\xbf\xbdst\xef\xbf\xbd" );

	// Incomplete UTF8 sequence
	text = "Test\xe6\x97";
	StringUtils::SanitizeUTF8(text);
	ASSERT_EQ( text, "Test\xef\xbf\xbd" );

	// Incomplete UTF8 sequence
	text = "Te\xe6\x97st";
	StringUtils::SanitizeUTF8(text);
	//Hexdump::Dump(stderr, "%s", text.data(), text.length());
	ASSERT_EQ( text, "Te\xef\xbf\xbdst" );

	// Nulls
	text = "Te\0\0st";
	StringUtils::SanitizeUTF8(text);
	ASSERT_EQ( text, "Te\0\0st" ); // technically, \0 is valid UTF8
}

TEST(StringUtilsTest, TestSanitizeFilePath)
{
	string text;

	// Path containing file://
	text = "file:///media/internal/something";
	StringUtils::SanitizeFilePath(text);
	ASSERT_EQ( "/media/internal/something", text );

	// Path that does not contain file://
	text = "/media/internal/something";
	StringUtils::SanitizeFilePath(text);
	ASSERT_EQ( "/media/internal/something", text );

	// Empty string
	text = "";
	StringUtils::SanitizeFilePath(text);
	ASSERT_EQ( "", text );
}

TEST(StringUtilsTest, TestSanitizeFileCacheName)
{
	string text;

	text = "a/b/c\\d.txt";
	StringUtils::SanitizeFileCacheName(text);
	ASSERT_EQ( "a_b_c_d.txt", text );
}
