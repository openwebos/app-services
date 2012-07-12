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

#include <gtest/gtest.h>
#include "mimeparser/Rfc822Tokenizer.h"
#include <string>

using namespace std;

TEST(Rfc822TokenizerTest, TestAtoms)
{
	Rfc822StringTokenizer tokenizer;
	Rfc822Token token;

	string data = "The quick brown   fox!";
	tokenizer.SetLine(data);

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_Atom, token.type );
	EXPECT_EQ( "The", token.value );

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_Atom, token.type );
	EXPECT_EQ( "quick", token.value );

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_Atom, token.type );
	EXPECT_EQ( "brown", token.value );

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_Atom, token.type );
	EXPECT_EQ( "fox!", token.value );

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_EOF, token.type );
}

TEST(Rfc822TokenizerTest, TestString)
{
	Rfc822StringTokenizer tokenizer;
	Rfc822Token token;

	string data = "Hello \"Wor\\\"ld\"";
	tokenizer.SetLine(data);

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_Atom, token.type );
	EXPECT_EQ( "Hello", token.value );

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_String, token.type );
	EXPECT_EQ( "Wor\"ld", token.value );
}

TEST(Rfc822TokenizerTest, TestComment)
{
	Rfc822StringTokenizer tokenizer;
	Rfc822Token token;

	string data = "Alpha (Moo) Beta";
	tokenizer.SetLine(data);

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_Atom, token.type );
	EXPECT_EQ( "Alpha", token.value );

	tokenizer.NextToken(token);
	EXPECT_EQ( Rfc822Token::Token_Atom, token.type );
	EXPECT_EQ( "Beta", token.value );
}
