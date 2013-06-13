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

#include "email/PreviewTextGenerator.h"
#include <gtest/gtest.h>

using namespace std;

TEST(PreviewTextGeneratorTest, TestStrip)
{
	string out = PreviewTextGenerator::GeneratePreviewText("A <style>foobar</style> B", 100, true);
	EXPECT_EQ( "A B", out );

	out = PreviewTextGenerator::GeneratePreviewText("A<br>B", 100, true);
	EXPECT_EQ( "A B", out );

	out = PreviewTextGenerator::GeneratePreviewText("A      B", 100, true);
	EXPECT_EQ( "A B", out );

	out = PreviewTextGenerator::GeneratePreviewText("A <a href", 100, true);
	EXPECT_EQ( "A", out );

	out = PreviewTextGenerator::GeneratePreviewText("A <span style=\"rgb(1,2,3)\"> B", 100, true);
	EXPECT_EQ( "A B", out );

	out = PreviewTextGenerator::GeneratePreviewText("A <b>B</b> C", 100, true);
	EXPECT_EQ( "A B C", out );
}

TEST(PreviewTextGeneratorTest, TestReplaceEntities)
{
	string out;

	out = PreviewTextGenerator::ReplaceEntities("Copyright &#169; 2010");
	EXPECT_EQ( "Copyright \xc2\xa9 2010", out );

	out = PreviewTextGenerator::ReplaceEntities("Copyright &#xA9; 2010");
	EXPECT_EQ( "Copyright \xc2\xa9 2010", out );

	out = PreviewTextGenerator::ReplaceEntities("Copyright &copy; 2010");
	EXPECT_EQ( "Copyright \xc2\xa9 2010", out );

	// Bad entity: null char
	out = PreviewTextGenerator::ReplaceEntities("Bad entity &#00;");
	EXPECT_EQ( "Bad entity &#00;", out );

	// Bad entity: foo is not a number
	out = PreviewTextGenerator::ReplaceEntities("Bad entity &#foo; and &#xfoo;");
	EXPECT_EQ( "Bad entity &#foo; and &#xfoo;", out );
}

TEST(PreviewTextGeneratorTest, TestTruncateUTF8)
{
	string text;

	// Nihongo (Japanese). Three characters which are three bytes each in UTF-8 form.
	text = "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e";
	PreviewTextGenerator::TruncateUTF8(text, 7);

	// Should get truncated to two characters with no incomplete UTF-8 sequences.
	EXPECT_EQ( "\xe6\x97\xa5\xe6\x9c\xac", text );
}
