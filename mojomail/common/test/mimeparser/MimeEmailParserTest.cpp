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

#include <gtest/gtest.h>
#include "mimeparser/MimeEmailParser.h"
#include <iostream>
#include <fstream>

using namespace std;

class DebugParseEventHandler : public ParseEventHandler
{
public:
	DebugParseEventHandler(ostream& out, ParseEventHandler* h = NULL) : h(h), out(out) {}

	virtual void HandleBeginEmail() { out << __func__ << endl; if (h) h->HandleBeginEmail(); }
	virtual void HandleEndEmail() { out << __func__ << endl; if (h) h->HandleEndEmail(); }

	virtual void HandleBeginPart() { out << __func__ << endl; if (h) h->HandleBeginPart(); }
	virtual void HandleEndPart() { out << __func__ << endl; if (h) h->HandleEndPart(); }

	virtual void HandleBeginHeaders() { out << __func__ << endl; if (h) h->HandleBeginHeaders(); }
	virtual void HandleEndHeaders(bool incomplete) { out << __func__ << endl; if (h) h->HandleEndHeaders(incomplete); }

	virtual void HandleBeginBody() { out << __func__ << endl; if (h) h->HandleBeginBody(); }
	virtual void HandleBodyData(const char* data, size_t length) { out << __func__ << ": " << std::string(data, length) << endl; if (h) h->HandleBodyData(data, length); }
	virtual void HandleEndBody(bool incomplete) { out << __func__ << endl; if (h) h->HandleEndBody(incomplete); }

	virtual void HandleContentType(const std::string& type, const std::string& subtype, const std::map<std::string, std::string>& parameters) { out << __func__ << " " << type << "/" << subtype << endl; if (h) h->HandleContentType(type, subtype, parameters); }
	virtual void HandleHeader(const std::string& fieldName, const std::string& fieldValue) { out << __func__ << " " << fieldName << ": " << fieldValue << endl; if (h) h->HandleHeader(fieldName, fieldValue); }

	ParseEventHandler* h;
	ostream& out;
};

class TestParseEventHandler : public ParseEventHandler
{
public:
	TestParseEventHandler()
	: beginEmailCount(0), endEmailCount(0), beginPartCount(0), endPartCount(0),
	  beginHeadersCount(0), endHeadersCount(0), beginBodyCount(0), endBodyCount(0) {}

	// TODO replace with gmock
	void CheckCounts()
	{
		// Check final counts
		EXPECT_EQ(beginEmailCount, endEmailCount);
		EXPECT_EQ(beginPartCount, endPartCount);
		EXPECT_EQ(beginHeadersCount, endHeadersCount);
		EXPECT_EQ(beginBodyCount, endBodyCount);
	}

	virtual void HandleBeginEmail() { beginEmailCount++; }
	virtual void HandleEndEmail() { endEmailCount++; }

	virtual void HandleBeginPart() { beginPartCount++; }
	virtual void HandleEndPart() { endPartCount++; }

	virtual void HandleBeginHeaders() { beginHeadersCount++; }
	virtual void HandleEndHeaders(bool incomplete) { endHeadersCount++; }

	virtual void HandleBeginBody() { beginBodyCount++; }
	virtual void HandleBodyData(const char* data, size_t length) { }
	virtual void HandleEndBody(bool incomplete) { endBodyCount++; }

	virtual void HandleContentType(const std::string& type, const std::string& subtype, const std::map<std::string, std::string>& parameters) { }
	virtual void HandleHeader(const std::string& fieldName, const std::string& fieldValue) { }

	int	beginEmailCount;
	int endEmailCount;
	int beginPartCount;
	int endPartCount;
	int beginHeadersCount;
	int endHeadersCount;
	int beginBodyCount;
	int endBodyCount;
};

void FeedChopped(MimeEmailParser& parser, const std::string& data, int chunkSize)
{
	size_t offset = 0;
	for (size_t endLength = chunkSize; endLength < data.length(); endLength += chunkSize) {
		string substring = data.substr(offset, endLength - offset);
		size_t bytesHandled = parser.ParseData(substring.data(), substring.length(), false);
		offset += bytesHandled;
	}

	// Handle any remaining data
	if (offset < data.length()) {
		offset += parser.ParseData(data.data() + offset, data.length() - offset, false);
	}

	EXPECT_EQ(data.length(), offset);
}

TEST (MimeEmailParserTest, TestBasic)
{
	TestParseEventHandler handler;
	MimeEmailParser parser(handler);

	parser.BeginEmail();
	parser.ParseLine("Subject: Test\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("BODY\r\n");
	parser.EndEmail();

	handler.CheckCounts();
}

TEST (MimeEmailParserTest, TestBulk)
{
	string data =
			"Subject: Test\r\n"
			"\r\n"
			"BODY\r\n";

	for (unsigned int stride = 1; stride <= data.length(); stride++) {
		//cout << "stride = " << stride << endl;

		TestParseEventHandler handler;
		//DebugParseEventHandler debugHandler(cerr, &handler);
		MimeEmailParser parser(handler);

		parser.BeginEmail();
		FeedChopped(parser, data, stride);
		parser.EndEmail();

		handler.CheckCounts();
	}
}

TEST (MimeEmailParserTest, TestBasicIncompleteHeader)
{
	TestParseEventHandler handler;
	//DebugParseEventHandler debugHandler(cerr, &handler);
	MimeEmailParser parser(handler);

	parser.BeginEmail();
	parser.ParseLine("Subject: Test\r\n");
	parser.EndEmail();

	handler.CheckCounts();
}

TEST (MimeEmailParserTest, TestBasicIncompleteHeader2)
{
	TestParseEventHandler handler;
	//DebugParseEventHandler debugHandler(cerr, &handler);
	MimeEmailParser parser(handler);

	parser.BeginEmail();
	parser.ParseLine("Subject: Test\r\n");
	parser.ParseLine("\r\n");
	parser.EndEmail();

	handler.CheckCounts();
}

TEST (MimeEmailParserTest, TestMultipart)
{
	TestParseEventHandler handler;
	MimeEmailParser parser(handler);

	parser.BeginEmail();
	parser.ParseLine("Subject: Test\r\n");
	parser.ParseLine("MIME-Version: 1.0\r\n");
	parser.ParseLine("Content-Type: multipart/alternative; boundary=BOUNDARY\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("--BOUNDARY\r\n");
	parser.ParseLine("Content-Type: text/plain\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("Text body here\r\n");
	parser.ParseLine("--BOUNDARY\r\n");
	parser.ParseLine("Content-Type: text/html\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("HTML body here\r\n");
	parser.ParseLine("--BOUNDARY--\r\n");
	parser.EndEmail();

	handler.CheckCounts();
}

TEST (MimeEmailParserTest, TestNestedMultipart)
{
	TestParseEventHandler handler;
	MimeEmailParser parser(handler);

	parser.BeginEmail();
	parser.ParseLine("Subject: Test\r\n");
	parser.ParseLine("MIME-Version: 1.0\r\n");
	parser.ParseLine("Content-Type: multipart/alternative; boundary=OUTER\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("--OUTER\r\n");
	parser.ParseLine("Content-Type: text/plain\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("Text body here\r\n");
	parser.ParseLine("--OUTER\r\n");
	parser.ParseLine("Content-Type: multipart/related; boundary=\"INNER\"\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("--INNER\r\n");
	parser.ParseLine("Content-Type: text/html\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("HTML body here\r\n");
	parser.ParseLine("--INNER--\r\n");
	parser.ParseLine("--OUTER--\r\n");
	parser.EndEmail();

	handler.CheckCounts();
}

TEST (MimeEmailParserTest, TestSpaceBeforeHeader)
{
	TestParseEventHandler handler;
	MimeEmailParser parser(handler);

	parser.BeginEmail();
	parser.ParseLine("Subject: Test\r\n");
	parser.ParseLine("MIME-Version: 1.0\r\n");
	parser.ParseLine("Content-Type: multipart/alternative; boundary=BOUNDARY\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("--BOUNDARY\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("Content-Type: text/plain\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("Text body here\r\n");
	parser.ParseLine("--BOUNDARY--\r\n");
	parser.EndEmail();

	handler.CheckCounts();
}

TEST (MimeEmailParserTest, TestNoEndingNewline)
{
	TestParseEventHandler handler;
	MimeEmailParser parser(handler);

	parser.BeginEmail();
	parser.ParseLine("Subject: Test\r\n");
	parser.ParseLine("\r\n");
	parser.ParseLine("Body goes here");
	parser.ParseLine("Another line");
	parser.EndEmail();

	handler.CheckCounts();
}
