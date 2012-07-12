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

#include "parser/SemanticActions.h"
#include "parser/Util.h"
#include "parser/Rfc3501Tokenizer.h"
#include "data/ImapEmail.h"
#include "data/EmailPart.h"
#include "data/EmailAddress.h"
#include "ImapPrivate.h"
#include "exceptions/Rfc3501ParseException.h"
#include "stream/ByteBufferOutputStream.h"
#include "email/Rfc2047Decoder.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/trim.hpp>
#include "util/StringUtils.h"

using namespace std;

SemanticActions::SemanticActions(Rfc3501Tokenizer& tokenizer)
: m_tokenizer(tokenizer),
  m_stringType(ST_Null),
  m_nzNumber(0),
  m_msgNum(0),
  m_expectBinaryData(false),
  m_binaryDataLength(0),
  m_flagsUpdated(false),
  m_sectionType(Section_None)
{
	m_bufferOutputStream.reset(new ByteBufferOutputStream());

	m_emailStack.push(NestedEmail());
	m_emailStack.top().m_email.reset(new ImapEmail());
}

SemanticActions::~SemanticActions()
{
}

void SemanticActions::readAheadLiteral(void) {
	int n = 0;
	bool ok = Util::from_string(n, m_tokenizer.value(), std::dec);
	if (ok) {
		m_expectBinaryData = true;
		m_binaryDataLength = n;
		//m_logger.print("readAheadLiteral: expect %d byte of binary data", n);

		if(m_currentOutputStream.get() == NULL) {
			m_bufferOutputStream->Clear();
			m_currentOutputStream = m_bufferOutputStream;
		}

		fprintf(stderr, "* expecting binary data\n");
	}
	else {
		// throw parser exception?
		throw Rfc3501ParseException("expected literal byte count", __FILE__, __LINE__);
	}
}

std::string SemanticActions::nullableString() {
	return m_stringValue;
}

void SemanticActions::saveLiteralString(void) {
	m_stringType = ST_Literal;
	m_stringValue.clear();
	m_stringValue = m_bufferOutputStream->GetBuffer();

	m_bufferOutputStream->Clear();
}

void SemanticActions::endLiteralBytes(void) {
	m_expectBinaryData = false;
	m_binaryDataLength = 0;
}

void SemanticActions::saveQuotedString(void) {
	//printf("*** saveQuotedString(): token=%s\n", m_tokenizer.value().c_str());
	m_stringType = ST_Quoted;
	m_stringValue = m_tokenizer.value();
}

void SemanticActions::saveAtomString(void) {
	//printf("*** saveAtomString(): token=%s\n", m_tokenizer.value().c_str());
	m_stringType = ST_Quoted;
	m_stringValue = m_tokenizer.value();
}

void SemanticActions::saveNullString(void) {
	//printf("*** saveNullString(): token=%s\n", m_tokenizer.value().c_str());
	m_stringType = ST_Null;
	m_stringValue.clear();
}

void SemanticActions::saveInternalDate(void) {
	// FIXME should be associated with INTERNALDATE instead of saving directly
	//GetCurrentEmail().m_internalDate = m_tokenizer.value();
	// Format: "16-Apr-2010 10:32:38 -0400"
	try {
		MojInt64 date = Util::ParseInternalDate(m_tokenizer.value());
		GetCurrentEmail().SetDateReceived(date);
	} catch(const Rfc3501ParseException& e) {
		// FIXME report error
	}
}

void SemanticActions::saveUID(void) {
	UID uid;
	if(Util::from_string(uid, m_tokenizer.value(), std::dec))
		GetCurrentEmail().SetUID(uid);
	// FIXME handle error
}

void SemanticActions::saveNonZeroNumber(void)
{
	MojUInt32 number;
	if(Util::from_string(number, m_tokenizer.value(), std::dec))
		m_nzNumber = number;
	else
		throw Rfc3501ParseException("not a valid non-zero number", __FILE__, __LINE__);
}

void SemanticActions::saveMsgNum(void) {
	m_msgNum = m_nzNumber;
}

// A phrase is a word, a quoted string, or a RFC 2047 encoded block.
string SemanticActions::ParsePhrase(const string& phrase)
{
	if(boost::starts_with(phrase, "=?") && boost::ends_with(phrase, "?=")) {
		try {
			string output;
			Rfc2047Decoder::Decode(phrase, output);
			return output;
		} catch(const exception& e) {
			// fall through
		}
	}

	string temp = phrase;
	StringUtils::SanitizeASCII(temp);
	return temp;
}

// Text may contain a mix of RFC 2047 and plain ASCII.
// RFC 2047 blocks are separated from ASCII by linear whitespace.
// Linear whitespace between two RFC 2047 blocks is ignored.
string SemanticActions::ParseText(const string& text)
{
	try {
		string out;
		Rfc2047Decoder::DecodeText(text, out);
		return out;
	} catch(const exception& e) {
		string temp = text;
		StringUtils::SanitizeASCII(temp);
		return temp;
	}
}

string SemanticActions::GetCurrentString(bool lowercase) const
{
	string value = m_stringValue;
	StringUtils::SanitizeASCII(value, "?");

	if (lowercase) {
		boost::to_lower(value);
	}

	return value;
}

void SemanticActions::setSection()
{
}

void SemanticActions::setHeadersSection()
{
	m_sectionType = Section_Headers;
}

void SemanticActions::expectBodyData(void) {
	fprintf(stderr, "Setting output stream to body output stream %p\n", m_bodyOutputStream.get());
	m_currentOutputStream = m_bodyOutputStream;
	m_stringValue.clear();
}

void SemanticActions::handleBodyData(void) {
	fprintf(stderr, "Closing body output stream\n");

	if(m_currentOutputStream.get()) {
		// If the body was an inline string rather than a literal, we'll append it now
		if(!m_stringValue.empty()) {
			m_currentOutputStream->Write(m_stringValue);
		}

		m_currentOutputStream->Flush();
		m_currentOutputStream->Close();
	}

	if(m_sectionType == Section_Headers) {
		// Parse headers
		ParseExtraHeaders(m_stringValue);
	}

	// Clear section after we're done
	m_sectionType = Section_None;

	m_currentOutputStream.reset();
	m_bodyOutputStream.reset();
}

void SemanticActions::ParseExtraHeader(const std::string& headerName, const std::string& value)
{
	if(boost::iequals(headerName, "X-Priority")) {
		Email::Priority priority = Email::Priority_Normal;

		if(value == "1" || value == "2") {
			priority = Email::Priority_High;
		} else if(value == "4" || value == "5") {
			priority = Email::Priority_Low;
		}

		GetCurrentEmail().SetPriority(priority);
	} else if(boost::iequals(headerName, "Importance")) {
		if( boost::iequals(value, "high") ) {
			GetCurrentEmail().SetPriority(Email::Priority_High);
		}
	}
}

void SemanticActions::ParseExtraHeaders(const std::string& headerString)
{
	try {
		size_t offset = 0;

		while(offset < headerString.length()) {
			size_t end = headerString.find("\r\n", offset);
			if(end != string::npos) {
				string line = headerString.substr(offset, end - offset);

				size_t colonPos = line.find(":");
				if(colonPos != string::npos && colonPos != line.size()) {
					string header = line.substr(0, colonPos);
					string value = line.substr(colonPos + 1);
					boost::trim(value);

					ParseExtraHeader(header, value);
				}

				offset = end + 2;
			} else {
				break;
			}
		}
	} catch(...) {
		// ignore parse errors
	}
}

void SemanticActions::beginNestedEmail()
{
	//fprintf(stderr, "begin nested\n");

	// Store current part
	GetNestedEmail().m_currentPart = m_currentPart;

	// Push new email
	m_emailStack.push(NestedEmail());
	m_emailStack.top().m_email.reset(new ImapEmail());
}

void SemanticActions::endNestedEmail()
{
	//fprintf(stderr, "end nested\n");

	// pop email
	m_emailStack.pop();

	// restore current part
	m_currentPart = GetNestedEmail().m_currentPart;
}

void SemanticActions::beginBodyStructure(void)
{

}

void SemanticActions::endBodyStructure(void)
{
	fprintf(stderr, "Parts:\n");
	EmailPartList emailParts;
	SerializeParts(emailParts, GetCurrentPart());

	bool hasAttachment = false;
	bool hasHtmlPart = false;

	BOOST_FOREACH(const EmailPartPtr& part, emailParts) {
		fprintf(stderr, "part %s %s charset=%s\n", part->GetSection().c_str(), part->GetMimeType().c_str(), part->GetCharset().c_str());

		if(part->IsAttachment()) {
			hasAttachment = true;
		}

		if(part->IsBodyPart() && boost::iequals(part->GetMimeType(), "text/html")) {
			hasHtmlPart = true;
		}
	}

	// Workaround for Verizon PictureMail, which has inline attachments with a contentId, but no HTML body
	if(!hasHtmlPart) {
		BOOST_FOREACH(const EmailPartPtr& part, emailParts) {
			if(part->IsInline() && !part->GetContentId().empty()) {
				part->SetType(EmailPart::ATTACHMENT);
				hasAttachment = true;
			}
		}
	}

	GetCurrentEmail().SetPartList(emailParts);
	GetCurrentEmail().SetHasAttachments(hasAttachment);

	m_currentPart.reset();
}

void SemanticActions::beginPartSet(void) {
	CreatePart();
	GetCurrentPart()->mimeType = "multipart";
	GetNestedEmail().m_multipartStack.push(m_currentPart);
}

void SemanticActions::endPartSet(void) {
	m_currentPart = GetNestedEmail().m_multipartStack.top();
	GetNestedEmail().m_multipartStack.pop();
}

void SemanticActions::beginPartList(void) {
}

void SemanticActions::endPartList(void) {
	m_currentPart = GetNestedEmail().m_multipartStack.top();
}

void SemanticActions::CreatePart()
{
	// Add part
	m_currentPart.reset(new ImapStructurePart());

	if(!GetNestedEmail().m_multipartStack.empty()) {
		ImapStructurePart& parent = *(GetNestedEmail().m_multipartStack.top());
		parent.subparts.push_back(m_currentPart);

		stringstream ss;
		if(!parent.partSection.empty())
			ss << parent.partSection << ".";
		ss << parent.subparts.size();

		m_currentPart->partSection = ss.str();
	}

	//fprintf(stderr, "Creating part %s @ %p\n", m_currentPart->partSection.c_str(), m_currentPart.get());

}

void SemanticActions::SerializeParts(EmailPartList& emailParts, const boost::shared_ptr<ImapStructurePart>& imapPart, int depth)
{
	// Prevent excessive recursion
	if(depth > 100) // FIXME use constant
		return;

	if(!imapPart->subparts.empty()) {
		assert( boost::iequals(imapPart->mimeType, "multipart") );

		if(boost::iequals(imapPart->mimeSubtype, "alternative")) {
			// Scan over subparts *backwards*; the last viewable part (possibly multipart) should be kept, all others discarded
			BOOST_FOREACH(const boost::shared_ptr<ImapStructurePart>& subpart, make_pair(imapPart->subparts.rbegin(), imapPart->subparts.rend())) {
				EmailPartList altParts;

				// Flatten this subpart into a list of parts (usually just one, but could be a multipart/related section)
				SerializeParts(altParts, subpart, depth + 1);

				bool hasBodyPart = false;

				// Check if the subpart, or any of the nested child parts, is a body part
				BOOST_FOREACH(const EmailPartPtr& emailPart, altParts) {
					if(emailPart->IsBodyPart()) {
						hasBodyPart = true;
						break;
					}
				}

				if(hasBodyPart) {

					emailParts.insert(emailParts.end(), altParts.begin(), altParts.end());
					break;
				} else {
					// Doesn't look like a body part. Attach parts anyway, but keep going.
					emailParts.insert(emailParts.end(), altParts.begin(), altParts.end());
				}
			}
		} else {
			BOOST_FOREACH(const boost::shared_ptr<ImapStructurePart>& subpart, imapPart->subparts) {
				SerializeParts(emailParts, subpart, depth + 1);
			}
		}


	} else {
		// normal email part
		//fprintf(stderr, "serializing part %p\n", imapPart.get());

		EmailPart::PartType type = EmailPart::INLINE;
		if( boost::iequals(imapPart->disposition, "attachment") ) {
			type = EmailPart::ATTACHMENT;
		}

		if(type == EmailPart::INLINE) {
			if(boost::iequals(imapPart->mimeType, "text")
			&& (boost::iequals(imapPart->mimeSubtype, "html")
			|| boost::iequals(imapPart->mimeSubtype, "plain")
			|| boost::iequals(imapPart->mimeSubtype, "enriched"))) {
				type = EmailPart::BODY;
			} else if(imapPart->contentId.empty() || !boost::iequals(imapPart->mimeType, "image")) {
				// not inline if it doesn't have a contentId, or isn't an image
				type = EmailPart::ATTACHMENT;
			}
		}

		EmailPartPtr emailPart(new EmailPart(type));
		emailPart->SetMimeType(imapPart->mimeType + "/" + imapPart->mimeSubtype);

		emailPart->SetContentId(imapPart->contentId);

		emailPart->SetEncoding(imapPart->encoding);
		emailPart->SetCharset(imapPart->charset);
		emailPart->SetDisplayName(imapPart->name);
		emailPart->SetEncodedSize(imapPart->size);
		emailPart->SetEstimatedSize(emailPart->EstimateSize());

		if(depth > 0) {
			emailPart->SetSection(imapPart->partSection);
		} else {
			// if this is the only part
			emailPart->SetSection("1");
		}

		emailParts.push_back(emailPart);
	}
}

void SemanticActions::setContentEncoding(void) {
	GetCurrentPart()->encoding = GetCurrentString(true);
}

void SemanticActions::setPartDisposition() {
	GetCurrentPart()->disposition = GetCurrentString(true);
}

void SemanticActions::setPartParameterName() {
	m_parameterName = GetCurrentString(true);

	m_stringValue.clear();
}

void SemanticActions::setPartParameterValue() {
	// Parameter name is lowercased earlier
	//fprintf(stderr, "parameter %s=%s\n", m_parameterName.c_str(), m_stringValue.c_str());

	//fprintf(stderr, "Current part %p\n", GetCurrentPart().get());

	if(m_parameterName == "charset") {
		GetCurrentPart()->charset = GetCurrentString(true);
	}
	else if (m_parameterName == "name") {
		GetCurrentPart()->name = ParseText(m_stringValue);
	}
	else if (m_parameterName == "filename") {
		GetCurrentPart()->name = ParseText(m_stringValue);
	}
}

void SemanticActions::setContentId() {
	string contentId = GetCurrentString(false);

	int sz = contentId.size();
	if (sz>=2 && contentId[0]=='<' && contentId[sz-1]=='>')
		GetCurrentPart()->contentId = contentId.substr(1, sz-2);
	else
		GetCurrentPart()->contentId = contentId;
}

void SemanticActions::setMimeType(void) {
	CreatePart();
	GetCurrentPart()->mimeType = GetCurrentString(true);
}

void SemanticActions::setMimeSubtype(void) {
	GetCurrentPart()->mimeSubtype = GetCurrentString(true);
}

// Due to funky parser logic for this workaround, the value stored in the mime type field
// is actually the subtype for an alternative part.
void SemanticActions::zeroPartWorkaround(void) {
	GetCurrentPart()->mimeSubtype = GetCurrentPart()->mimeType;
	GetCurrentPart()->mimeType = "alternative";
}

void SemanticActions::setMimeTypeMessage(void) {
	CreatePart();
	GetCurrentPart()->mimeType = "message";
}

void SemanticActions::setMimeTypeText(void) {
	CreatePart();
	GetCurrentPart()->mimeType = "text";
}

void SemanticActions::setMimeSubtypeRfc822(void) {
	GetCurrentPart()->mimeSubtype = "rfc822";
}

void SemanticActions::saveMediaSubtype() {
	//fprintf(stderr, "media subtype: %s\n", m_stringValue.c_str());

	GetCurrentPart()->mimeSubtype = GetCurrentString(true);
}

void SemanticActions::beginFlags(void) {
	m_flagsUpdated = true;
}

void SemanticActions::keywordFlagAtom(void) {
	//string flag = m_tokenizer.value();
}

void SemanticActions::systemFlagAtom(void) {
	string flag = m_tokenizer.valueUpper();

	if(boost::iequals(flag, "Seen")) {
		GetCurrentEmail().SetRead(true);
	} else if(boost::iequals(flag, "Deleted")) {
		GetCurrentEmail().SetDeleted(true);
	} else if(boost::iequals(flag, "Answered")) {
		GetCurrentEmail().SetReplied(true);
	} else if(boost::iequals(flag, "Flagged")) {
		GetCurrentEmail().SetFlagged(true);
	}
}

void SemanticActions::setPartSize(void) {
	size_t n = 0;
	bool ok = Util::from_string(n, m_tokenizer.value(), std::dec);
	if (ok) {
		GetCurrentPart()->size = n;
	}
	else {
		throw Rfc3501ParseException("error parsing part size", __FILE__, __LINE__);
	}
}

void SemanticActions::endFetch(void) {
}

void SemanticActions::envSubject() {
	//printf("SemanticActions.envSubject: &GetCurrentEmail()=%08x, m_currentPart=%08x\n", int(&GetCurrentEmail()), int(m_currentPart));
	GetCurrentEmail().SetSubject(ParseText(m_stringValue));
}

void SemanticActions::envSenderAddressList() {
}

void SemanticActions::envFromAddressList() {
	if(!m_envelope.GetCurrentAddressList()->empty()) {
		// get first address
		GetCurrentEmail().SetFrom(m_envelope.GetCurrentAddressList()->front());
	}
}

void SemanticActions::envToAddressList() {
	GetCurrentEmail().SetTo(m_envelope.GetCurrentAddressList());
}

void SemanticActions::envReplyToAddressList() {
	if(!m_envelope.GetCurrentAddressList()->empty()) {
		// get first address
		GetCurrentEmail().SetReplyTo(m_envelope.GetCurrentAddressList()->front());
	}
}

void SemanticActions::envBccAddressList() {
	GetCurrentEmail().SetBcc(m_envelope.GetCurrentAddressList());
}

void SemanticActions::envCcAddressList() {
	GetCurrentEmail().SetCc(m_envelope.GetCurrentAddressList());
}

void SemanticActions::envBeginAddressList() {
	// Allocate a new list
	m_envelope.m_addressList.reset(new EmailAddressList());
}

void SemanticActions::envDate() {
}

void SemanticActions::envInReplyTo() {
	GetCurrentEmail().SetInReplyTo( GetCurrentString(false) );
}

void SemanticActions::envMessageId() {
	GetCurrentEmail().SetMessageId( GetCurrentString(false) );
}

void SemanticActions::envEndAddress() {
	EmailAddressPtr address = make_shared<EmailAddress>(m_envelope.m_addressDisplayName, m_envelope.m_addressEmail);
	m_envelope.GetCurrentAddressList()->push_back(address);
}

void SemanticActions::envAddrName() {
	m_envelope.m_addressDisplayName = ParsePhrase(m_stringValue);
}

void SemanticActions::envAddrAdl() {
	// this is unused at the moment, but might be in the future
}

void SemanticActions::envAddrMailbox() {
	m_envelope.m_addressEmail = GetCurrentString(false);
}

void SemanticActions::envAddrHost() {
	m_envelope.m_addressEmail.append("@");
	m_envelope.m_addressEmail.append( GetCurrentString(false) );
}
