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

#include "mimeparser/EmailHeaderFieldParser.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "mimeparser/Rfc822Tokenizer.h"
#include <iostream>
#include "email/Rfc2047Decoder.h"
#include "email/DateUtils.h"
#include "data/Email.h"
#include "data/EmailAddress.h"
#include "data/EmailPart.h"
#include "util/StringUtils.h"

using namespace std;

void EmailHeaderFieldParser::ParseTextField(const std::string& line, std::string& outValue)
{
	outValue = Rfc2047Decoder::SafeDecodeText(line);
}

// Input: tokenizer (semicolon not yet consumed)
// Output: parameter map (lower-case parameter names)
void EmailHeaderFieldParser::ParseFieldParameters(Rfc822StringTokenizer& tokenizer, ParameterMap& outParameters)
{
	// Find semicolon
	Rfc822Token token;

	while (tokenizer.NextToken(token) && token.IsSymbol(';')) {
		string parameterName, parameterValue;

		// Get parameter name
		if (tokenizer.NextToken(token, Rfc822TokenizerFlags::Tokenizer_ParameterMode) && token.IsAtom()) {
			parameterName = token.value;
		} else {
			throw HeaderParseException("invalid parameter name");
		}

		// Get equals
		if (tokenizer.NextToken(token, Rfc822TokenizerFlags::Tokenizer_ParameterMode) && token.IsSymbol('=')) {
			// ignore
		} else {
			throw HeaderParseException("invalid parameter =");
		}

		if (tokenizer.NextToken(token, Rfc822TokenizerFlags::Tokenizer_ParameterMode)) {
			if (token.IsWord()) {
				parameterValue = token.value;
			} else if (token.IsSymbol(';')) {
				// just in case; handle empty parameter value
				tokenizer.Unpeek();
			}
		}

		boost::to_lower(parameterName);
		outParameters[parameterName] = parameterValue;
	}
}

void EmailHeaderFieldParser::ParseContentTypeField(const std::string& line, std::string& outType, std::string& outSubtype, ParameterMap& outParameters)
{
	Rfc822StringTokenizer tokenizer;
	Rfc822Token token;

	if (line.empty()) return;

	tokenizer.SetLine(line);

	// Read content type
	if (tokenizer.NextToken(token, Rfc822TokenizerFlags::Tokenizer_ParameterMode) && token.IsAtom()) {
		outType = token.value;
	} else {
		throw HeaderParseException("invalid type");
	}

	// Read '/'
	if (tokenizer.NextToken(token, Rfc822TokenizerFlags::Tokenizer_ParameterMode) && token.IsSymbol('/')) {
		// Read subtype
		if (tokenizer.NextToken(token, Rfc822TokenizerFlags::Tokenizer_ParameterMode) && token.IsAtom()) {
			outSubtype = token.value;
		}
	} // otherwise, skip to parameters

	ParseFieldParameters(tokenizer, outParameters);
}

void EmailHeaderFieldParser::ParseContentDispositionField(const std::string& line, std::string& outDisposition, ParameterMap& outParameters)
{
	Rfc822StringTokenizer tokenizer;
	Rfc822Token token;

	if (line.empty()) return;

	tokenizer.SetLine(line);

	// Read content disposition
	if (tokenizer.NextToken(token, Rfc822TokenizerFlags::Tokenizer_ParameterMode) && token.IsAtom()) {
		outDisposition = token.value;
	} else {
		throw HeaderParseException("invalid disposition");
	}

	ParseFieldParameters(tokenizer, outParameters);
}

/*
 * RFC 2822 address specification
 *
 * address         =       mailbox / group
 * mailbox         =       name-addr / addr-spec
 * name-addr       =       [display-name] angle-addr
 * angle-addr      =       [CFWS] "<" addr-spec ">" [CFWS] / obs-angle-addr
 * group           =       display-name ":" [mailbox-list / CFWS] ";"
 *                        [CFWS]
 * display-name    =       phrase
 * mailbox-list    =       (mailbox *("," mailbox)) / obs-mbox-list
 * address-list    =       (address *("," address)) / obs-addr-list
 *
 */
void EmailHeaderFieldParser::ParseAddress(Rfc822StringTokenizer& tokenizer, EmailAddressList& outAddressList, bool mailboxOnly)
{
	Rfc822Token token;

	// Read displayName
	// This is hack since we don't parse a character at a time
	string displayName;
	string text;

	while (true) {
		tokenizer.NextToken(token);

		if (token.IsWord()) {
			// Add text to a buffer so we can RFC 2047 decode it easily
			if (!text.empty()) text.push_back(' '); // add space
			text.append(token.value);
		} else {
			// Append any buffered text
			displayName.append(Rfc2047Decoder::SafeDecodeText(text));
			text.clear();

			if (token.type == Rfc822Token::Token_Symbol && !token.IsSymbol("<,:;")) {
				// treat most symbols as part of the name
				displayName.append(token.value);
			} else {
				tokenizer.Unpeek();
				break;
			}
		}
	}

	// Get next token: should be '<' or ':' or EOL
	tokenizer.NextToken(token);

	if (token.IsSymbol('<')) {
		// Fast and loose parsing of addr-spec
		string addressSpec;

		while (tokenizer.NextToken(token) && !token.IsSymbol('>')) {
			addressSpec.append(token.value);
		}

		if (!addressSpec.empty()) {
			// Add address to list
			StringUtils::SanitizeASCII(addressSpec);
			EmailAddressPtr address(new EmailAddress(displayName, addressSpec));
			outAddressList.push_back(address);
		}
	} else if (!mailboxOnly && token.IsSymbol(':')) {
		// Parse group mailbox list
		ParseAddressList(tokenizer, outAddressList, true);
	} else {
		// Other symbol -- return to caller

		if (!displayName.empty()) {
			// Display name is actually address. Add to list.
			EmailAddressPtr address(new EmailAddress(displayName));
			outAddressList.push_back(address);
		}

		tokenizer.Unpeek();
	}
}

void EmailHeaderFieldParser::ParseAddressList(Rfc822StringTokenizer& tokenizer, EmailAddressList& outAddressList, bool mailboxOnly)
{
	Rfc822Token token;

	// Parse mailbox list
	do {
		ParseAddress(tokenizer, outAddressList, mailboxOnly);

		// read a token; should be ',', ';', or EOF
		tokenizer.NextToken(token);
	} while (!token.IsEnd() && (mailboxOnly || !token.IsSymbol(';')));
}

void EmailHeaderFieldParser::ParseAddressListField(const std::string& line, EmailAddressList& outAddressList)
{
	Rfc822StringTokenizer tokenizer;
	Rfc822Token token;

	if (line.empty()) return;

	tokenizer.SetLine(line);

	ParseAddressList(tokenizer, outAddressList, false);
}

void EmailHeaderFieldParser::ParseDateField(const std::string& line, time_t& time)
{
	time = DateUtils::ParseRfc822Date(line.c_str());
}

bool EmailHeaderFieldParser::ParseEmailHeaderField(Email& email, const string& fieldNameLower, const string& fieldValue)
{
	if (fieldNameLower == "subject") {
		// decode subject
		string subject;
		ParseTextField(fieldValue, subject);
		email.SetSubject(subject);
	} else if (fieldNameLower == "date") {
		time_t date;
		ParseDateField(fieldValue, date);

		// Technically this is the date sent, not received
		email.SetDateReceived( MojInt64(date) * 1000L);
	} else if (fieldNameLower == "from" || fieldNameLower == "reply-to" || fieldNameLower == "to" || fieldNameLower == "cc" || fieldNameLower == "bcc") {
		EmailAddressListPtr addressListPtr(new EmailAddressList());
		ParseAddressListField(fieldValue, *addressListPtr);

		if (fieldNameLower == "to") {
			email.SetTo(addressListPtr);
		} else if (fieldNameLower == "cc") {
			email.SetCc(addressListPtr);
		} else if (fieldNameLower == "bcc") {
			email.SetBcc(addressListPtr);
		} else if (fieldNameLower == "from") {
			// get first address
			if (!addressListPtr->empty()) {
				email.SetFrom(addressListPtr->at(0));
			}
		} else if (fieldNameLower == "reply-to") {
			// get first address
			if (!addressListPtr->empty()) {
				email.SetReplyTo(addressListPtr->at(0));
			}
		}
	} else if (fieldNameLower == "in-reply-to") {
		email.SetInReplyTo( StringUtils::GetSanitizedASCII(fieldValue) );
	} else if (fieldNameLower == "x-priority") {
		if(fieldValue == "1" || fieldValue == "2") {
			email.SetPriority(Email::Priority_High);
		} else if (fieldValue == "4" || fieldValue == "5") {
			email.SetPriority(Email::Priority_Low);
		}
	} else {
		// no match
		return false;
	}

	return true;
}

bool EmailHeaderFieldParser::ParsePartHeaderField(EmailPart& part, const string& fieldNameLower, const string& fieldValue)
{
	if (fieldNameLower == "content-transfer-encoding") {
		part.SetEncoding( StringUtils::GetSanitizedASCII(fieldValue) );
	} else if (fieldNameLower == "content-disposition") {
		EmailHeaderFieldParser::ParameterMap params;

		string disposition;
		ParseContentDispositionField(fieldValue, disposition, params);

		if (boost::iequals(disposition, "attachment")) {
			part.SetType(EmailPart::ATTACHMENT);
		}

		// Get name
		string filename = params["filename"];

		if (!filename.empty()) {
			filename = Rfc2047Decoder::SafeDecodeText(filename);
			part.SetDisplayName(filename);
		}
	} else if (fieldNameLower == "content-id") {
		string contentId = fieldValue;
		size_t sz = contentId.length();

		if (sz > 2 && contentId[0] == '<' && contentId[sz-1] == '>')
			contentId = contentId.substr(1, sz-2);

		part.SetContentId( StringUtils::GetSanitizedASCII(contentId) );
	} else {
		return false;
	}

	return true;
}

