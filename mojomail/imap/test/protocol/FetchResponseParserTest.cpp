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

#include "protocol/FetchResponseParser.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <sstream>
#include "client/MockImapSession.h"
#include "protocol/MockDoneSlot.h"
#include "data/ImapEmail.h"
#include "data/EmailAddress.h"
#include "MockTestSetup.h"
#include "data/EmailPart.h"
#include <fstream>
#include "data/ImapEmailAdapter.h"
#include "ImapPrivate.h"
#include <gtest/gtest.h>

using namespace std;

static inline string reformat_quoted(const string& s)
{
	string temp = s;

	boost::trim(temp);
	boost::erase_first(temp, "(");
	boost::erase_last(temp, ")");
	boost::trim(temp);
	boost::replace_all(temp, "\n", " ");

	return temp;
}

#define QUOTE_IMAP(x) (reformat_quoted(#x))
#define TEST_ALL 1

void dump_json(ImapEmail& email)
{
	MojObject obj;
	ImapEmailAdapter::SerializeToDatabaseObject(email, obj);
	fprintf(stderr, "email object: %s", AsJsonString(obj).c_str());
}

void BuildEnvelope(string date, string subject, string from, string sender,
		string replyTo, string to, string cc, string bcc, string inReplyTo, string messageId)
{
	stringstream ss;
	ss << date << " " << subject << " " << from << " " << sender << " " << replyTo << "";
	ss << to << " " << cc << " " << bcc << " " << inReplyTo << " " << messageId;
}

TEST(FetchResponseParserTest, TestSimpleHeaders)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("FETCH 10 ALL", parser);

	string line = QUOTE_IMAP((
		* 10 FETCH (FLAGS (\\Seen) INTERNALDATE "16-Apr-2010 10:32:38 -0400"
		RFC822.SIZE 8382 ENVELOPE ("Fri, 16 Apr 2010 10:32:37 -0400" "Subject"
		(("From Person" NIL "from" "example.com"))
		(("Sender Person" NIL "sender" "example.com"))
		(("ReplyTo Person" NIL "replyto" "example.com"))
		(("To Person" NIL "to" "example.com"))
		NIL NIL NIL
		"<messageid>"))
	));

	is->FeedLine(line);

	const vector<FetchUpdate> updates = parser->GetUpdates();
	ASSERT_EQ( (size_t) 1, updates.size() );
	ImapEmail* email = updates.at(0).email.get();

	ASSERT_EQ( "Subject", email->GetSubject() );
#endif
}

TEST(FetchResponseParserTest, TestLiteralHeaders)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("FETCH 11 (ENVELOPE)", parser);

	string subject = "This is a subject";
	string toName = "To Person";
	string toDomain = "example.org";

	stringstream ss;
	ss << "* 1 FETCH (UID 1234 ENVELOPE (NIL {" << subject.length() << "}\r\n" << subject;
	ss << " ((\"From Person\" NIL \"from\" \"example.com\"))";
	ss << " NIL NIL"; // sender, reply-to
	ss << " (({" << toName.length() << "}\r\n" << toName; // to
	ss << " NIL \"to.person\" {" << toDomain.length() << "}\r\n";

	is->Feed(ss.str());
	is->Feed(toDomain);
	is->FeedLine(")) NIL NIL NIL \"<messageid>\"))"); // cc bcc in-reply-to message-id

	is->FlushBuffer();

	const vector<FetchUpdate>& emails = parser->GetUpdates();
	ASSERT_EQ( (size_t) 1, emails.size() );
	ImapEmail* email = emails.at(0).email.get();

	ASSERT_EQ( subject, email->GetSubject() );
	ASSERT_EQ( (size_t) 1, email->GetTo()->size() );
	ASSERT_EQ( toName, email->GetTo()->at(0)->GetDisplayName() );
#endif
}

const boost::shared_ptr<ImapEmail>& GetOneEmail(const MojRefCountedPtr<FetchResponseParser>& parser)
{
	parser->CheckStatus();

	const vector<FetchUpdate>& emails = parser->GetUpdates();

	//ASSERT_EQ( (size_t) 1, emails.size() );

	const FetchUpdate& update = emails.at(0);

	if(update.email.get() == NULL) {
		throw MailException("no email", __FILE__, __LINE__);
	}

	return update.email;
}

boost::shared_ptr<ImapEmail> ParseOneEmail(string line)
{
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 1024 (BODYSTRUCTURE)", parser);

	is->FeedLine(line);
	is->FeedLine("~A1 OK");
	is->FlushBuffer();

	return GetOneEmail(parser);
}

// Basic body structure parsing
TEST(FetchResponseParserTest, TestBodyStructure)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 1024 (BODYSTRUCTURE)", parser);

	string line = QUOTE_IMAP((
		* 1 FETCH (UID 1024
		BODYSTRUCTURE (("TEXT" "PLAIN" ("CHARSET" "iso-8859-1") NIL NIL "QUOTED-PRINTABLE" 2518 94 NIL NIL NIL)("TEXT" "HTML" ("CHARSET" "iso-8859-1") NIL NIL "QUOTED-PRINTABLE" 8504 162 NIL NIL NIL) "ALTERNATIVE" ("BOUNDARY" "======1272494251854======") NIL NIL))
	));

	is->FeedLine(line);
	is->FeedLine("~A1 OK");
	is->FlushBuffer();

	boost::shared_ptr<ImapEmail> email = GetOneEmail(parser);

	// Only the HTML part is kept since it's multipart/alternative
	const EmailPartList& parts = email->GetPartList();

	ASSERT_EQ( parts.size(), (size_t) 1 );
	ASSERT_EQ( parts.at(0)->GetCharset(), "iso-8859-1" );
	ASSERT_EQ( parts.at(0)->GetEncoding(), "quoted-printable" );
	ASSERT_EQ( parts.at(0)->GetEncodedSize(), 8504 );
#endif
}

// Test illegal body structure sent by Gmail for message/rfc822 parts (appears as a regular body-type-basic)
TEST(FetchResponseParserTest, TestMessageRfc822Gmail)
{
#if TEST_ALL
	string line = QUOTE_IMAP((
		* 1 FETCH (UID 1024
		BODYSTRUCTURE (("TEXT" "HTML" NIL NIL NIL "7BIT" 13 1 NIL NIL NIL)("MESSAGE" "RFC822" NIL NIL NIL "7BIT" 227 NIL ("ATTACHMENT" ("FILENAME" "test.eml")) NIL) "MIXED" ("BOUNDARY" "BOUNDARY") NIL NIL))
	));

	boost::shared_ptr<ImapEmail> email = ParseOneEmail(line);

	// The text/html part and the message/rfc822 attachment
	const EmailPartList& parts = email->GetPartList();

	ASSERT_EQ( parts.size(), (size_t) 2 );
	ASSERT_TRUE( parts.at(0)->IsBodyPart() );
	ASSERT_TRUE( parts.at(1)->IsAttachment() );
	ASSERT_TRUE( parts.at(1)->GetDisplayName() == "test.eml" );
#endif
}

// Test nested rfc822 email
TEST(FetchResponseParserTest, TestMessageRfc822)
{
#if TEST_ALL
	string line = QUOTE_IMAP((
	* 1 FETCH (UID 1024
	ENVELOPE (NIL "Test rfc822" ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) NIL NIL NIL NIL)
	BODYSTRUCTURE (("TEXT" "HTML" NIL NIL NIL "7BIT" 13 1 NIL NIL NIL NIL)("MESSAGE" "RFC822" NIL NIL NIL "7BIT" 227 (NIL "Test attached email" ((NIL NIL "username" "palm.com")) ((NIL NIL "username" "palm.com")) ((NIL NIL "username" "palm.com")) ((NIL NIL "username" "palm.com")) NIL NIL NIL NIL) (("TEXT" "HTML" NIL NIL NIL "7BIT" 22 1 NIL NIL NIL NIL) "ALTERNATIVE" ("BOUNDARY" "INNER") NIL NIL NIL) 12 NIL ("attachment" ("FILENAME" "test.eml")) NIL NIL) "MIXED" ("BOUNDARY" "BOUNDARY") NIL NIL NIL))
	));

	boost::shared_ptr<ImapEmail> email = ParseOneEmail(line);

	ASSERT_EQ( email->GetSubject(), "Test rfc822" );

	EmailAddressListPtr toAddresses = email->GetTo();
	ASSERT_EQ( toAddresses->size(), (size_t) 1 );
	ASSERT_EQ( toAddresses->at(0)->GetAddress(), "username@gmail.com" );

	// Body part and a message/rfc822 attachment
	EmailPartList parts = email->GetPartList();
	ASSERT_EQ( parts.size(), (size_t) 2 );

	EmailPartPtr& bodyPart = parts.at(0);
	ASSERT_EQ( bodyPart->GetMimeType(), "text/html" );
	ASSERT_TRUE( bodyPart->IsBodyPart() );
	ASSERT_EQ( bodyPart->GetEncodedSize(), 13 );

	EmailPartPtr& messagePart = parts.at(1);
	ASSERT_EQ( messagePart->GetMimeType(), "message/rfc822" );
	ASSERT_TRUE( messagePart->IsAttachment() );
	ASSERT_EQ( messagePart->GetEncodedSize(), 227 );

	//fprintf(stderr, "Display name %s\n", messagePart->GetDisplayName().c_str() );
	ASSERT_EQ( messagePart->GetDisplayName(), "test.eml" );

#endif
}

// Multiple levels of nesting
TEST(FetchResponseParserTest, TestNestedEmail)
{
#if TEST_ALL
	string line = QUOTE_IMAP((
		* 1 FETCH (UID 1024
		BODYSTRUCTURE (("TEXT" "HTML" NIL NIL NIL "7BIT" 100 2 NIL NIL NIL NIL)("MESSAGE" "RFC822" NIL NIL NIL "7BIT" 469 (NIL "FW: Totally Awesome" ((NIL NIL "username" "palm.com")) ((NIL NIL "username" "palm.com")) ((NIL NIL "username" "palm.com")) ((NIL NIL "username" "palm.com")) NIL NIL NIL NIL) (("TEXT" "HTML" NIL NIL NIL "7BIT" 17 1 NIL NIL NIL NIL)("MESSAGE" "RFC822" NIL NIL NIL "7BIT" 122 (NIL "Totally Awesome" ((NIL NIL "test" "example.com")) ((NIL NIL "test" "example.com")) ((NIL NIL "test" "example.com")) ((NIL NIL "tester" "example.com")) NIL NIL NIL NIL) ("TEXT" "PLAIN" ("CHARSET" "us-ascii") NIL NIL "7BIT" 20 1 NIL NIL NIL NIL) 6 NIL ("attachment" ("FILENAME" "fw-fw.eml")) NIL NIL) "MIXED" ("BOUNDARY" "INNER") NIL NIL NIL) 24 NIL ("attachment" ("FILENAME" "test.eml")) NIL NIL) "MIXED" ("BOUNDARY" "BOUNDARY") NIL NIL NIL)
		ENVELOPE (NIL "FW: FW: Totally Awesome" ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) NIL NIL NIL NIL))
	));

	boost::shared_ptr<ImapEmail> email = ParseOneEmail(line);

	ASSERT_EQ( email->GetSubject(), "FW: FW: Totally Awesome" );

	EmailAddressListPtr toAddresses = email->GetTo();
	ASSERT_EQ( toAddresses->size(), (size_t) 1 );
	ASSERT_EQ( toAddresses->at(0)->GetAddress(), "username@gmail.com" );

	// Body part and a message/rfc822 attachment
	EmailPartList parts = email->GetPartList();
	ASSERT_EQ( parts.size(), (size_t) 2 );

	EmailPartPtr& bodyPart = parts.at(0);
	ASSERT_EQ( bodyPart->GetMimeType(), "text/html" );
	ASSERT_TRUE( bodyPart->IsBodyPart() );

	EmailPartPtr& messagePart = parts.at(1);
	ASSERT_EQ( messagePart->GetMimeType(), "message/rfc822" );
	ASSERT_TRUE( messagePart->IsAttachment() );
	ASSERT_EQ( messagePart->GetDisplayName(), "test.eml" );
#endif
}

TEST(FetchResponseParserTest, TestYahooEmlAttachments)
{
#if 0 // Yahoo server bug
	string line = QUOTE_IMAP((
		* 1 FETCH (BODYSTRUCTURE (("text" "plain" ("charset" "ISO-8859-1" " format" "flowed") NIL NIL "7bit" 6 1 NIL NIL NIL NIL)("message" "rfc822" ("name" "Some attachments.eml") NIL NIL "7bit" 208177 ("Fri, 10 Sep 2010 11:22:05 -0700" "Some attachments" (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) NIL NIL NIL "<4C8A774D.30307@gmail.com>") (("text" "plain" ("charset" "ISO-8859-1" " format" "flowed") NIL NIL "7bit" 4 1 NIL NIL NIL NIL)("image" "png" ("name" "accounts_2010-21-07_185928.png") NIL NIL "base64" 98864 NIL ("inline" ("filename" "accounts_2010-21-07_185928.png")) NIL NIL)("image" "png" ("name" "eas-145.png") NIL NIL "base64" 52296 NIL ("inline" ("filename" "eas-145.png")) NIL NIL)("image" "png" ("name" "eas-main.png") NIL NIL "base64" 55582 NIL ("inline" ("filename" "eas-main.png")) NIL NIL) "mixed" ("boundary" "------------030400010902000606030807") NIL ) 2840 NIL ("inline" ("filename" "eas-main.png")) NIL NIL) "mixed" ("boundary" "------------000106010706010908000801") NIL ) ENVELOPE ("Fri, 10 Sep 2010 11:27:17 -0700" "[Fwd: Some attachments]" (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) ((NIL NIL "username2" "yahoo.com")) NIL NIL NIL "<4C8A7885.9040404@gmail.com>"))
	));

	boost::shared_ptr<ImapEmail> email = ParseOneEmail(line);

	dump_json(*email);
#endif
}

// Test message/delivery-status part
TEST(FetchResponseParserTest, TestMessageDeliveryStatus)
{
#if TEST_ALL
	string line = QUOTE_IMAP((
		* 1 FETCH (UID 1024
		BODYSTRUCTURE (("TEXT" "HTML" NIL NIL NIL "7BIT" 13 1 NIL NIL NIL NIL)("MESSAGE" "DELIVERY-STATUS" NIL NIL NIL "7BIT" 6 NIL NIL NIL NIL) "MIXED" ("BOUNDARY" "BOUNDARY") NIL NIL NIL))
	));

	ParseOneEmail(line);
#endif
}

// make sure that MESSAGE_IN_QUOTES can still appear as a regular string
TEST(FetchResponseParserTest, TestTokenInQuotes)
{
#if TEST_ALL
	string line = QUOTE_IMAP((
		* 1 FETCH (UID 1024
		ENVELOPE ("Wed, 4 Aug 2010 02:10:15 -0700" "message" (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) NIL NIL NIL "<messageid>"))
	));

	boost::shared_ptr<ImapEmail> email = ParseOneEmail(line);
	//fprintf(stderr, "subject: %s\n", email->GetSubject().c_str());
	ASSERT_EQ( email->GetSubject(), "message" );
#endif
}

// Test handling of buggy Gmail behavior when a subject line contains \r\n in the middle (NOV-87436)
TEST(FetchResponseParserTest, TestGmailBrokenLine)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 1024 (BODYSTRUCTURE)", parser);

	string line = QUOTE_IMAP((
			* 1 FETCH (UID 1024 ENVELOPE ("Tue, 5 Jan 2010 12:41:34 -0800 (PST)" "The quick brown fox jumped over the lazy dog" ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) ((NIL NIL "username" "gmail.com")) NIL NIL NIL "<3465387674237263106@unknownmsgid>") BODYSTRUCTURE ("TEXT" "PLAIN" NIL NIL NIL "7BIT" 9 1 NIL NIL NIL))
	));

	// split line after the 'j' in jumped; a newline will be inserted in the middle
	string line1 = line.substr( 0, line.find("umpe") );
	string line2 = line.substr( line.find("umpe") );

	is->FeedLine(line1);
	is->FeedLine(line2);

	//fprintf(stderr, "line 1: [%s]\n", line1.c_str());
	//fprintf(stderr, "line 2: [%s]\n", line2.c_str());

	is->FeedLine("~A1 OK");
	is->FlushBuffer();

	const Email& email = *GetOneEmail(parser);
	//fprintf(stderr, "email subject: [%s]\n", email.GetSubject().c_str());
	ASSERT_EQ( email.GetSubject(), "The quick brown fox j\r\numped over the lazy dog" );
#endif
}

// Test handling of buggy Yahoo behavior with extra space after the filename (NOV-112816)
TEST(FetchResponseParserTest, TestYahooExtraSpace)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 1024 (BODYSTRUCTURE)", parser);

	string line = QUOTE_IMAP((
		* 1234 FETCH (BODYSTRUCTURE (("text" "plain" ("charset" "iso-8859-1") NIL NIL "7bit" 870 31 NIL NIL NIL "attachment.htm" )("text" "html" ("charset" "us-ascii") NIL NIL "7bit" 2285 6 NIL NIL NIL "attachment.htm" ) "alternative" ("boundary" "----BOUNDARY") NIL ) ENVELOPE ("Sat, 01 Aug 2010 01:01:01 -0700" "Subject" (("DisplayName" NIL "Name" "Domain")) (("DisplayName" NIL "Name" "Domain")) (("DisplayName" NIL "Name" "Domain")) (("DisplayName" NIL "Name" "Domain")) NIL NIL NIL "<messageid>") FLAGS (\\Seen) INTERNALDATE "01-Sat-2010 01:01:01 +0000" UID 12345)
	));

	is->FeedLine(line);

	is->FeedLine("~A1 OK");
	is->FlushBuffer();

	const Email& email = *GetOneEmail(parser);
	//fprintf(stderr, "email subject: [%s]\n", email.GetSubject().c_str());
	ASSERT_EQ( email.GetSubject(), "Subject" );
#endif
}

// Test parsing GMX email sent from Gmail (NOV-114378)
TEST(FetchResponseParserTest, TestGmxGmail)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 1024 (BODYSTRUCTURE)", parser);

	string line = QUOTE_IMAP((
		* 3 FETCH (UID 3 FLAGS (\\Seen) INTERNALDATE " 1-Sep-2010 07:41:29 +0000" ENVELOPE ("Wed, 1 Sep 2010 00:41:28 -0700" "Test from Gmail" (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) ((NIL NIL "username" "gmx.com")) NIL NIL NIL "<AANLkTikO5jp73tOVKZaT2OumxsaFkFv84M8g3r9OYt_g@mail.gmail.com>") BODYSTRUCTURE (("text" "plain" ("charset" "UTF-8") NIL NIL NIL 7 1 NIL NIL NIL NIL)("text" "html" ("charset" "UTF-8") NIL NIL NIL 11 1 NIL NIL NIL NIL) "alternative" ("boundary" "00504501427351c79a048f2dd333") NIL NIL NIL))
	));

	is->FeedLine(line);

	is->FeedLine("~A1 OK");
	is->FlushBuffer();

	const Email& email = *GetOneEmail(parser);
	ASSERT_EQ( email.GetPartList().size(), (size_t) 1 );
#endif
}

TEST(FetchResponseParserTest, TestPriority)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 1024 (BODY.PEEK[HEADER.FIELDS (X-PRIORITY IMPORTANCE)])", parser);

	stringstream data;
	data << "* 1 FETCH (UID 1024 BODY[HEADER.FIELDS (X-PRIORITY IMPORTANCE)] ";
	string headers = "X-Priority: 1\r\nImportance: high\r\n\r\n";
	data << "{" << headers.size() << "}\r\n" << headers;
	data << ")\r\n";

	is->Feed(data.str());

	is->FeedLine("~A1 OK");
	is->FlushBuffer();

	const Email& email = *GetOneEmail(parser);
	ASSERT_EQ( email.GetPriority(), Email::Priority_High );
#endif
}

TEST(FetchResponseParserTest, TestNewsletter)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 16592 (UID FLAGS INTERNALDATE ENVELOPE BODYSTRUCTURE BODY.PEEK[HEADER.FIELDS (X-PRIORITY IMPORTANCE)])", parser);

	is->Feed("* 1865 FETCH (");

	string line = QUOTE_IMAP((
		UID 16592 INTERNALDATE "22-Dec-2010 00:43:02 +0000" FLAGS () ENVELOPE ("Wed, 22 Dec 2010 00:15:28 +0000" "Newsletter subject" (("Newsletter" NIL "no-reply" "example.org")) (("Newsletter" NIL "no-reply" "example.org")) (("Newsletter" NIL "no-reply" "example.org")) ((NIL NIL "username" "gmail.com")) NIL NIL NIL "<182282256630734534706@example.net>") BODYSTRUCTURE (("TEXT" "PLAIN" ("CHARSET" "ISO-8859-1") NIL NIL "7BIT" 1195 29 NIL NIL NIL)("TEXT" "HTML" ("CHARSET" "ISO-8859-1") NIL NIL "7BIT" 4846 135 NIL NIL NIL) "ALTERNATIVE" ("BOUNDARY" "----AGJNG_9303_0.430396692595324") NIL NIL) BODY[HEADER.FIELDS (X-PRIORITY IMPORTANCE)] {17}
	));

	is->FeedLine(line);

	is->Feed("X-Priority: 3\r\n\r\n");
	is->FeedLine(")");

	is->FeedLine("~A1 OK");
	is->FlushBuffer();
#endif
}

#if 0
TEST(FetchResponseParserTest, TestFromFile)
{
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 1024 (ENVELOPE BODYSTRUCTURE)", parser);

	ifstream stream("testmail.txt", ifstream::in);

	while(stream.good()) {
		string line;
		getline(stream, line);
		is->FeedLine(line);
	}

	is->FeedLine("~A1 OK");
	is->FlushBuffer();

	parser->CheckStatus();
}
#endif

TEST(FetchResponseParserTest, TestDelivery)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 16592 (UID FLAGS INTERNALDATE ENVELOPE BODYSTRUCTURE BODY.PEEK[HEADER.FIELDS (X-PRIORITY IMPORTANCE)])", parser);

	//is->Feed("* 1865 FETCH (");

	string line = QUOTE_IMAP((
		* 9315 FETCH (UID 18990 INTERNALDATE "07-May-2011 03:47:58 +0000" ENVELOPE ("Fri, 6 May 2011 20:47:58 -0700 (PDT)" "Re: Re: FW: Subject" ((NIL NIL "noreply" "example.com")) ((NIL NIL "noreply" "example.com")) ((NIL NIL "username3" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) NIL NIL NIL "<913133060.323598.1304740078102.JavaMail.prod@app010.dmz>") BODYSTRUCTURE ((("TEXT" "PLAIN" ("CHARSET" "us-ascii") NIL NIL "7BIT" 481 20 NIL ("INLINE" NIL) NIL)("TEXT" "HTML" ("CHARSET" "us-ascii") NIL NIL "7BIT" 1259 41 NIL ("INLINE" NIL) NIL) "ALTERNATIVE" ("BOUNDARY" "----=_Part_323596_529173644.1304740078099") ("INLINE" NIL) NIL)("MESSAGE" "DELIVERY-STATUS" NIL NIL "Delivery report" "7BIT" 296 NIL NIL NIL)("MESSAGE" "RFC822" NIL NIL "Undelivered Message" "7BIT" 2687 ("Fri, 6 May 2011 20:47:37 -0700" "Re: FW: my last sunset at palm (now with correct photo album link)" (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) (("Person Name" NIL "username" "gmail.com")) ((NIL NIL "username2" "gmail.com")) NIL NIL "<C9EA0E53.F3FD%username@palm.com>" "<BANLkTimFmki7Gr9tY-F5-XorjtoaPCsjDA@mail.gmail.com>") ("ALTERNATIVE" ("BOUNDARY" "20cf307f37c8ad482004a2a77770") NIL NIL) 50 NIL NIL NIL) "REPORT" ("BOUNDARY" "----=_Part_323597_1355088785.1304740078099" "REPORT-TYPE" "delivery-status") NIL NIL))
	));

	is->FeedLine(line);
	is->FeedLine("~A1 OK");
	is->FlushBuffer();
#endif
}

TEST(FetchResponseParserTest, TestDelivery2)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 16592 (UID FLAGS INTERNALDATE ENVELOPE BODYSTRUCTURE BODY.PEEK[HEADER.FIELDS (X-PRIORITY IMPORTANCE)])", parser);

	string line = QUOTE_IMAP((
		* 58 FETCH (BODYSTRUCTURE ((("TEXT" "PLAIN" ("CHARSET" "us-ascii") NIL NIL "7BIT" 433 20 NIL ("INLINE" NIL) NIL)("TEXT" "HTML" ("CHARSET" "us-ascii") NIL NIL "7BIT" 1210 41 NIL ("INLINE" NIL) NIL) "ALTERNATIVE" ("BOUNDARY" "----=_Part_588821_1929170139.1304880014424") ("INLINE" NIL) NIL)("MESSAGE" "DELIVERY-STATUS" NIL NIL "Delivery report" "7BIT" 287 NIL NIL NIL)("MESSAGE" "RFC822" NIL NIL "Undelivered Message" "7BIT" 2213 ("Sun, 8 May 2011 11:40:07 -0700" "Subject of original email" (("Name" NIL "name" "gmail.com")) (("Name" NIL "username2" "gmail.com")) (("Name" NIL "name" "gmail.com")) ((NIL NIL "name" "name")) NIL NIL "NIL" "<msgid>") ("ALTERNATIVE" ("BOUNDARY" "bcaec52155a757fdc604a2c80da0") NIL NIL) 39 NIL NIL NIL) "REPORT" ("BOUNDARY" "----=_Part_588822_854423767.1304880014424" "REPORT-TYPE" "delivery-status") NIL NIL))
	));

	is->FeedLine(line);
	is->FeedLine("~A1 OK");
	is->FlushBuffer();
#endif
}

TEST(FetchResponseParserTest, TestNoAlternatives)
{
#if TEST_ALL
	MockTestSetup setup;
	MockImapSession& session = setup.GetSession();

	const MockInputStreamPtr& is = session.GetMockInputStream();

	MockDoneSlot slot;
	MojRefCountedPtr<FetchResponseParser> parser(new FetchResponseParser(session, slot.GetSlot()));

	session.SendRequest("UID FETCH 16592 (UID FLAGS INTERNALDATE ENVELOPE BODYSTRUCTURE BODY.PEEK[HEADER.FIELDS (X-PRIORITY IMPORTANCE)])", parser);

	string line = QUOTE_IMAP((
			* 2536 FETCH (BODYSTRUCTURE ("ALTERNATIVE" ("BOUNDARY" "XYZZY") NIL NIL))
	));

	is->FeedLine(line);
	is->FeedLine("~A1 OK");
	is->FlushBuffer();
#endif
}
