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

#include "email/Rfc2047Decoder.h"
#include <gtest/gtest.h>

using namespace std;

TEST(Rfc2047DecoderTest, TestDecodeText)
{
	// regular text
	if(true) {
		string input = "regular text";
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		EXPECT_EQ(output, "regular text");
	}

	// entire string encoded
	if(true) {
		string input = "=?ISO-8859-1?Q?Hello_World?=";
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		EXPECT_EQ(output, "Hello World");
	}

	// encoded block with preceding whitespace
	if(true) {
		string input = "\n  =?ISO-8859-1?Q?Hello_World?=";
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		fprintf(stderr, "[%s]\n", output.c_str());
		EXPECT_EQ(output, "\n  Hello World");
	}

	// encoded block with trailing text
	if(true) {
		string input = "=?ISO-8859-1?Q?Hello_World?= XYZ";
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		EXPECT_EQ(output, "Hello World XYZ");
	}

	// two encoded blocks separated by ASCII text
	if(true) {
		string input = "=?ISO-8859-1?Q?Very?= Berry =?UTF-8?Q?Smoothie?=";
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		EXPECT_EQ(output, "Very Berry Smoothie");
	}

	// two encoded blocks with no separation
	if(true) {
		string input = "=?ISO-8859-1?Q?abc?= =?UTF-8?Q?def?=";
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		fprintf(stderr, "[%s]\n", output.c_str());
		EXPECT_EQ(output, "abcdef");
	}
}

TEST(Rfc2047DecoderTest, TestDecodeIllegalText)
{
	// no terminator
	if(true) {
		string input = "=?UTF-8?Q?abc";
		string output;
		Rfc2047Decoder::DecodeText(input, output); // unchanged

		EXPECT_EQ(output, input);
	}

	// no linear whitespace
	if(true) {
		string input = "x=?UTF-8?Q?abc?=y";
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		// we don't handle this per spec right now
	}

	string sample = "=?ISO-8859-1?Q?Very?= Berry =?UTF-8?Q?Smoothie?=";
	for(size_t i = 1; i < sample.size() - 1; ++i) {
		string input = sample.substr(0, i);
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		// just make sure no exceptions are thrown
	}
}

TEST(Rfc2047DecoderTest, TestDecodeBadCharset)
{
	if(true) {
		string input = "=?not_a_charset?Q?foobar?=";
		string output;
		Rfc2047Decoder::DecodeText(input, output);

		EXPECT_EQ(output, input); // unchanged
	}
}

TEST(Rfc2047DecoderTest, TestSubject)
{
	string input = "=?ISO-8859-2?Q?=A3_=B3_to_start_\"Subject\"_line?=";
	string output;

	Rfc2047Decoder::DecodeText(input, output);
}

TEST(Rfc2047DecoderTest, TestEmpty)
{
	string input = "=?ISO-8859-1?Q?\?=";
	string output;

	Rfc2047Decoder::DecodeText(input, output);

	EXPECT_EQ(output, "");

	input = "x=?UTF-8?B?\?=y";

	Rfc2047Decoder::DecodeText(input, output);

	EXPECT_EQ(output, "xy");
}
