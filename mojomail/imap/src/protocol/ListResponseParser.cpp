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

#include "protocol/ListResponseParser.h"
#include <string>
#include <boost/algorithm/string/predicate.hpp>
#include "parser/Rfc3501Tokenizer.h"
#include "parser/Util.h"
#include "data/ImapFolder.h"
#include <boost/shared_array.hpp>
#include <boost/make_shared.hpp>
#include <unicode/ucnv.h>
#include "client/ImapSession.h"
#include "util/StringUtils.h"

using namespace std;

ListResponseParser::ListResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
: BufferedResponseParser(session, doneSlot)
{
}

ListResponseParser::~ListResponseParser()
{
}

/*
 * Example LIST response:
 * 
 * LIST (\HasNoChildren) "/" "Drafts"
 * LIST (\HasNoChildren) "/" "INBOX"
 * LIST (\HasNoChildren) "/" "UTF7 &MN0wsTDiMPM-"
 * LIST (\HasNoChildren) "/" "Sent"
 * LIST (\HasNoChildren) "/" "Trash"
 * LIST (\Noselect \HasChildren) "/" "[Gmail]"
 * LIST (\HasNoChildren) "/" "[Gmail]/All Mail"
 * LIST (\HasNoChildren) "/" "[Gmail]/Drafts"
 * LIST (\HasNoChildren) "/" "[Gmail]/Sent Mail"
 * LIST (\HasChildren \HasNoChildren) "/" "[Gmail]/Spam"
 * LIST (\HasNoChildren) "/" "[Gmail]/Starred"
 * LIST (\HasChildren \HasNoChildren) "/" "[Gmail]/Trash"
 */

void ListResponseParser::ParseFolder(const string& line, ImapFolder& folder)
{
	Rfc3501Tokenizer t(line);

	if (t.next() != TK_TEXT || (!t.match("LIST") && !t.match("XLIST")))
		ThrowParseException("expected LIST");
	if (t.next() != TK_SP)
		ThrowParseException("expected sp");
	if (t.next() != TK_LPAREN)
		ThrowParseException("expected (");

	t.next();
	for (;;) {
		if (t.tokenType == TK_RPAREN)
			break;
		if (t.tokenType == TK_TEXT)
		{
			// FIXME
			//folder.addAttribute(t.valueUpper());
			t.next();
		}
		else if (t.tokenType == TK_BACKSLASH) {
			t.next();
			if (t.tokenType != TK_TEXT)
				ThrowParseException("folder attribute expected");

			string attribute = t.valueUpper();

			if(attribute == "NOSELECT") {
				folder.SetSelectable(false);
			} else if(attribute == ImapFolder::XLIST_ALLMAIL || attribute == ImapFolder::XLIST_INBOX || attribute == ImapFolder::XLIST_DRAFTS
					|| attribute == ImapFolder::XLIST_SENT || attribute == ImapFolder::XLIST_SPAM || attribute == ImapFolder::XLIST_TRASH) {
				folder.SetXlistType(attribute);
			}
		}
		if (t.next() != TK_SP)
			break;
		t.next();
	}
	if (t.tokenType != TK_RPAREN)
		ThrowParseException("expected )");
	if (t.next() != TK_SP)
		ThrowParseException("expected sp");
	t.next();
	
	if (t.tokenType == TK_QUOTED_STRING)
		folder.SetDelimiter(t.value());
	else if(t.tokenType == TK_NIL || (t.tokenType == TK_TEXT && t.value() == Rfc3501Tokenizer::NIL_STRING))
		folder.SetDelimiter("");
	else
		ThrowParseException("expected delimiter");

	if (t.next() != TK_SP)
		ThrowParseException("expected sp");

	TokenType folderNameToken = t.next();
	if (folderNameToken != TK_QUOTED_STRING && folderNameToken != TK_TEXT)
		ThrowParseException("expected foldername");

	string folderName = t.value();
	StringUtils::SanitizeASCII(folderName, "?");
	
	folder.SetFolderName(folderName);
	
	// Decode UTF-7 folder name
	string displayName;

	try {
		size_t bufSize = folderName.size() * 4;
		boost::shared_array<char> buf(new char[bufSize]);

		// Decode using ICU's modified UTF-7 decoder, which is designed for IMAP folder names
		UErrorCode error = U_ZERO_ERROR;
		size_t length = ucnv_convert("UTF-8", "IMAP-mailbox-name", buf.get(), bufSize, folderName.data(), folderName.length(), &error);

		if(!U_FAILURE(error)) {
			// length could be larger than the buffer if it overflowed; use min(bufSize, length) to be safe
			displayName = string(buf.get(), min(bufSize, length));
		} else {
			MojLogWarning(m_log, "error decoding modified UTF-7 folder name");
			displayName = folderName;
		}
	} catch(const exception& e) {
		MojLogWarning(m_log, "error parsing modified UTF-7 folder name");
		displayName = folderName;
	}

	// Generate the display name
	folder.SetDisplayName(displayName);
	
	// If it's a child folder, strip out the parent folder names
	if(folder.GetDelimiter().length() > 0) {
		size_t index = displayName.rfind(folder.GetDelimiter());
		if(index != string::npos)
			folder.SetDisplayName( displayName.substr(index + 1) );
	}

	// FIXME
	//if (t.next() != TK_CR)
	//	ThrowParseException("expected endline");
}

bool ListResponseParser::HandleUntaggedResponse(const string& line)
{
	if(boost::istarts_with(line, "LIST") || boost::istarts_with(line, "XLIST")) {
		m_buffer = line;

		CheckResponseReady();

		return true;
	}
	
	return false;
}

void ListResponseParser::ResponseLineReady()
{
	ImapFolderPtr folder = boost::make_shared<ImapFolder>();
	ParseFolder(m_buffer, *folder);

	MojLogDebug(m_log, "parsed folder \"%s\"", folder->GetFolderName().c_str());
	m_folders.push_back(folder);
}
