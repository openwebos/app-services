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

#include "data/EmailAdapter.h"
#include "data/Email.h"
#include "data/EmailAddress.h"
#include "data/EmailPart.h"
#include "data/DatabaseAdapter.h"
#include "exceptions/MojErrException.h"
#include "data/EmailSchema.h"
#include "CommonPrivate.h"

using namespace std;
using namespace EmailSchema;

const char * const EmailAdapter::DRAFT_TYPE_NEW 			= "new";
const char * const EmailAdapter::DRAFT_TYPE_REPLY			= "reply";
const char * const EmailAdapter::DRAFT_TYPE_FORWARD			= "forward";
const char * const EmailAdapter::PRIORITY_LOW 				= "low";
const char * const EmailAdapter::PRIORITY_NORMAL			= "normal";
const char * const EmailAdapter::PRIORITY_HIGH				= "high";
const char * const EmailAdapter::HEADER_IMPORTANCE_HIGH		= "high";
const char * const EmailAdapter::HEADER_IMPORTANCE_LOW		= "low";
const char * const EmailAdapter::HEADER_PRIORITY_HIGHEST	= "1 Highest";
const char * const EmailAdapter::HEADER_PRIORITY_HIGH		= "2 High";
const char * const EmailAdapter::HEADER_PRIORITY_NORMAL		= "3 Normal";
const char * const EmailAdapter::HEADER_PRIORITY_LOW		= "4 Low";
const char * const EmailAdapter::HEADER_PRIORITY_LOWEST		= "5 Lowest";



void EmailAdapter::ParseDatabaseObject(const MojObject& obj, Email& email)
{
	MojErr err;
	
	MojObject folderId;
	err = obj.getRequired(FOLDER_ID, folderId);
	ErrorToException(err);
	email.SetFolderId(folderId);
	
	MojString subject;
	err = obj.getRequired(SUBJECT, subject);
	ErrorToException(err);
	email.SetSubject( std::string(subject) );
	
	MojObject fromAddress;
	err = obj.getRequired(FROM, fromAddress);
	ErrorToException(err);
	email.SetFrom( ParseAddress(fromAddress) );
	
	// Optional replyTo address
	MojObject replyTo;
	if(obj.get(REPLY_TO, replyTo) && !replyTo.null()) {
		email.SetReplyTo( ParseAddress(replyTo) );
	}

	MojInt64 timestamp;
	err = obj.getRequired(TIMESTAMP, timestamp);
	ErrorToException(err);
	email.SetDateReceived(timestamp);
	
	// Parse recipients
	MojObject recipients;
	err = obj.getRequired(RECIPIENTS, recipients);
	ErrorToException(err);
	ParseRecipients(recipients, email);
	
	// Parse flags
	MojObject flags;
	if (obj.get(FLAGS, flags)) {
		ParseFlags(flags, email);
	}

	// Parse parts
	MojObject parts;
	err = obj.getRequired(PARTS, parts);
	ErrorToException(err);

	EmailPartList partList;
	ParseParts(parts, partList);
	email.SetPartList(partList);

	MojObject originalMsgId;
	if (obj.get(ORIGINAL_MSG_ID, originalMsgId)) {
		email.SetOriginalMsgId(originalMsgId);
	}

	MojString draftType;
	bool hasDraftType = false;
	err = obj.get(DRAFT_TYPE, draftType, hasDraftType);
	ErrorToException(err);
	if (hasDraftType) {
		email.SetDraftType( ParseDraftType(draftType) );
	}

	MojString priority;
	bool hasPriority = false;
	err = obj.get(PRIORITY, priority, hasPriority);
	ErrorToException(err);
	// If no priority exists, this will default to normal
	email.SetPriority(ParsePriority(priority));

	email.SetMessageId( DatabaseAdapter::GetOptionalString(obj, MESSAGE_ID) );
	email.SetInReplyTo( DatabaseAdapter::GetOptionalString(obj, IN_REPLY_TO) );

	// SendStatus
	// NOTE: This is not being serialized back to the database in SerializeToDatabaseObject
	MojObject sendStatus;
	if (obj.get(SEND_STATUS, sendStatus)) {
		bool isSent = false;
		if (sendStatus.get(SendStatus::SENT, isSent)) {
			email.SetSent(isSent);
		}
		bool hasFatalError = false;
		if (sendStatus.get(SendStatus::FATAL_ERROR, hasFatalError)) {
			email.SetHasFatalError(hasFatalError);
		}
		MojInt64 retryCount;
		if (sendStatus.get(SendStatus::RETRY_COUNT, retryCount)) {
			email.SetRetryCount(retryCount);
		}
		MojObject sendError;
		if (sendStatus.get(SendStatus::ERROR, sendError)) {
			MojObject errorCode;
			if (sendError.get(SendStatusError::ERROR_CODE, errorCode) && !errorCode.null() && !errorCode.undefined()) {
				email.SetSendError(static_cast<MailError::ErrorCode>(errorCode.intValue()));
			}
		}
	}
}

EmailAddressPtr EmailAdapter::ParseAddress(const MojObject& addressObj)
{
	MojErr err;
	
	MojString addr;
	err = addressObj.getRequired(Address::ADDR, addr);
	ErrorToException(err);
	
	MojString name;
	bool hasName;
	err = addressObj.get(Address::NAME, name, hasName);
	ErrorToException(err);
	
	// WORKAROUND: some parts of the system expect a displayable name, even if only the
	// address is available. If the name is equal to the address, assume there wasn't
	// actually a real name available.
	if(name == addr) {
		hasName = false;
	}

	if(hasName)
		return boost::make_shared<EmailAddress>( std::string(name), std::string(addr) );
	else
		return boost::make_shared<EmailAddress>( std::string(addr) );
}

void EmailAdapter::ParseRecipients(const MojObject& recipients, Email& email)
{
	MojErr err;
	
	EmailAddressListPtr to_list(new EmailAddressList);
	EmailAddressListPtr cc_list(new EmailAddressList);
	EmailAddressListPtr bcc_list(new EmailAddressList);
	
	MojObject::ConstArrayIterator it = recipients.arrayBegin();

	for (; it != recipients.arrayEnd(); ++it) {
		const MojObject& recipient = *it;

		MojString type;
		err = recipient.getRequired(Part::TYPE, type);
		ErrorToException(err);
		
		EmailAddressPtr addr = ParseAddress(recipient);

		if(type.compareCaseless("to") == 0)
			to_list->push_back(addr);
		else if(type.compareCaseless("cc") == 0)
			cc_list->push_back(addr);
		else if(type.compareCaseless("bcc") == 0)
			bcc_list->push_back(addr);
		else // not a valid recipient type
			throw MailException("invalid recipient type", __FILE__, __LINE__);
	}
	
	email.SetTo(to_list);
	email.SetCc(cc_list);
	email.SetBcc(bcc_list);
}

Email::DraftType EmailAdapter::ParseDraftType(const MojString& draftType)
{
	if (draftType == DRAFT_TYPE_REPLY) {
		return Email::DraftTypeReply;
	} else if (draftType == DRAFT_TYPE_FORWARD) {
		return Email::DraftTypeForward;
	} else {
		return Email::DraftTypeNew;
	}
}

Email::Priority EmailAdapter::ParsePriority(const MojString& priority)
{
	if (priority == PRIORITY_LOW) {
		return Email::Priority_Low;
	} else if (priority == PRIORITY_HIGH) {
		return Email::Priority_High;
	} else {
		return Email::Priority_Normal;
	}
}

void EmailAdapter::ParseFlags(const MojObject& flags, Email& email)
{
	bool editedOriginal;
	if (flags.get(Flags::EDITEDORIGINAL, editedOriginal)) {
		email.SetEditedOriginal( editedOriginal );
	}

	// TODO: flagged
	// TODO: replied
	// TODO: forwarded
	// TODO: editedDraft
}

EmailPartPtr EmailAdapter::ParseEmailPart(const MojObject& partObj)
{
	MojErr err;

	MojString type;
	err = partObj.getRequired(Part::TYPE, type);
	ErrorToException(err);

	EmailPartPtr emailPart = boost::make_shared<EmailPart>( GetPartType(type) );

	MojObject id;
	err = partObj.getRequired(DatabaseAdapter::ID, id);
	ErrorToException(err);
	emailPart->SetId(id);

	emailPart->SetLocalFilePath( DatabaseAdapter::GetOptionalString(partObj, Part::PATH) );
	emailPart->SetDisplayName( DatabaseAdapter::GetOptionalString(partObj, Part::NAME) );
	emailPart->SetContentId( DatabaseAdapter::GetOptionalString(partObj, Part::CONTENT_ID) );
	emailPart->SetMimeType( DatabaseAdapter::GetOptionalString(partObj, Part::MIME_TYPE) );
	emailPart->SetEncoding( DatabaseAdapter::GetOptionalString(partObj, Part::ENCODING) );
	emailPart->SetCharset( DatabaseAdapter::GetOptionalString(partObj, Part::CHARSET) );
	emailPart->SetSection( DatabaseAdapter::GetOptionalString(partObj, Part::SECTION) );

	if(partObj.contains(Part::ESTIMATED_SIZE)) {
		MojInt64 estimatedSize = -1;
		err = partObj.getRequired(Part::ESTIMATED_SIZE, estimatedSize);
		emailPart->SetEstimatedSize(estimatedSize);
	}

	if(partObj.contains(Part::ENCODED_SIZE)) {
		MojInt64 encodedSize = -1;
		err = partObj.getRequired(Part::ENCODED_SIZE, encodedSize);
		emailPart->SetEncodedSize(encodedSize);
	}

	return emailPart;
}

void EmailAdapter::ParseParts(const MojObject& partsArray, EmailPartList& partList)
{
	MojObject::ConstArrayIterator it = partsArray.arrayBegin();

	for (; it != partsArray.arrayEnd(); ++it) {
		const MojObject& partObj = *it;

		EmailPartPtr emailPart = ParseEmailPart(partObj);

		// Add to the part list
		partList.push_back(emailPart);
	}
}

// NOTE: This should currently only be used for new emails
void EmailAdapter::SerializeToDatabaseObject(const Email& email, MojObject& obj)
{
	MojErr err;
	
	// Set the object kind
	err = obj.putString(KIND, Kind::EMAIL);
	ErrorToException(err);
	
	// FIXME object ID
	
	// Set the folder ID
	err = obj.put(FOLDER_ID, email.GetFolderId());
	ErrorToException(err);
	
	// Set flags
	MojObject flags;
	SerializeFlags(email, flags);

	err = obj.put(FLAGS, flags);
	ErrorToException(err);
	
	// The following fields only exist for a new e-mail to be added to the DB
	// If the e-mail object already exists in the DB, we shouldn't overwrite these fields
	// FIXME: Except for drafts. Maybe this logic should be moved elsewhere?
	if (true /*!obj.Exists()*/) {
		// Subject
		err = obj.putString(SUBJECT, email.GetSubject().c_str());
		ErrorToException(err);

		// Preview text
		err = obj.putString(SUMMARY, email.GetPreviewText().c_str());
		ErrorToException(err);
		
		// Timestamp in UTC milliseconds
		err = obj.put(TIMESTAMP, email.GetDateReceived());
		ErrorToException(err);
		
		// From address
		MojObject from;
		if(email.GetFrom().get()) // may not have a from address
			SerializeAddress(Address::Type::FROM, email.GetFrom(), from);
		err = obj.put(FROM, from);
		ErrorToException(err);
		
		// Reply-To address
		MojObject replyTo;
		if(email.GetReplyTo().get()) { // may not have a reply-to address
			SerializeAddress(Address::Type::REPLY_TO, email.GetReplyTo(), replyTo);
			err = obj.put(REPLY_TO, replyTo);
			ErrorToException(err);
		}

		// Recipients
		MojObject recipients;
		SerializeRecipients(Address::Type::TO, email.GetTo(), recipients);
		SerializeRecipients(Address::Type::CC, email.GetCc(), recipients);
		SerializeRecipients(Address::Type::BCC, email.GetBcc(), recipients);
		err = obj.put(RECIPIENTS, recipients);
		ErrorToException(err);
		
		// Parts
		MojObject parts;
		SerializeParts(email.GetPartList(), parts);
		err = obj.put(PARTS, parts);
		ErrorToException(err);

		// MessageId and InReplyTo
		DatabaseAdapter::PutOptionalString(obj, MESSAGE_ID, email.GetMessageId());
		DatabaseAdapter::PutOptionalString(obj, IN_REPLY_TO, email.GetInReplyTo());

		// Priority
		Email::Priority priority = email.GetPriority();
		if(priority == Email::Priority_High) {
			err = obj.putString(PRIORITY, PRIORITY_HIGH);
			ErrorToException(err);
		} else if(priority == Email::Priority_Low) {
			err = obj.putString(PRIORITY, PRIORITY_LOW);
			ErrorToException(err);
		}
	}
}

void EmailAdapter::SerializeAddress(const char* type, EmailAddressPtr address, MojObject& recipient)
{
	MojErr err;
	
	if( address.get() == NULL ) {
		throw MailException("attempted to serialize null address", __FILE__, __LINE__);
	}

	err = recipient.putString(Address::TYPE, type);
	ErrorToException(err);
	
	err = recipient.putString(Address::ADDR, address->GetAddress().c_str());
	ErrorToException(err);
	
	if(address->HasDisplayName()) {
		err = recipient.putString(Address::NAME, address->GetDisplayName().c_str());
		ErrorToException(err);
	} else {
		err = recipient.putString(Address::NAME, address->GetAddress().c_str());
		ErrorToException(err);
	}
}

void EmailAdapter::SerializeRecipients(const char* type, const EmailAddressListPtr& list, MojObject& recipients)
{
	EmailAddressList::const_iterator it;
	
	for(it = list->begin(); it != list->end(); ++it) {
		EmailAddressPtr address = *it;
		if (!address.get()) {
			throw MailException("Unable to serialize email: one of recipients is null", __FILE__, __LINE__);
		}

		MojObject recipient;
		SerializeAddress(type, address, recipient);
		MojErr err = recipients.push(recipient);
		ErrorToException(err);
	}
}

EmailPart::PartType EmailAdapter::GetPartType(const MojString& type)
{
	if(type == Part::Type::BODY)
		return EmailPart::BODY;
	else if(type == Part::Type::ATTACHMENT)
		return EmailPart::ATTACHMENT;
	else if(type == Part::Type::INLINE)
		return EmailPart::INLINE;
	else if(type == Part::Type::ALT_TEXT)
		return EmailPart::ALT_TEXT;
	else if(type == Part::Type::SMART_TEXT)
		return EmailPart::SMART_TEXT;
	
	throw MailException("invalid part type", __FILE__, __LINE__);
}

const char* EmailAdapter::GetPartTypeAsString(const EmailPartPtr& part)
{
	switch(part->GetType()) {
	case EmailPart::BODY:		return Part::Type::BODY;
	case EmailPart::ATTACHMENT:	return Part::Type::ATTACHMENT;
	case EmailPart::INLINE:		return Part::Type::INLINE;
	case EmailPart::SMART_TEXT:	return Part::Type::SMART_TEXT;
	case EmailPart::ALT_TEXT:	return Part::Type::ALT_TEXT;
	default: throw MailException("invalid part type", __FILE__, __LINE__);
	}
}

void EmailAdapter::SerializeParts(const EmailPartList& partsList, MojObject& partsArrayObj)
{
	EmailPartList::const_iterator it;
	
	for(it = partsList.begin(); it != partsList.end(); ++it) {
		EmailPartPtr emailPart = *it;
		if (!emailPart.get()) {
			throw MailException("Unable to serialize email: one of the parts is null", __FILE__, __LINE__);
		}

		MojObject partObj;
		MojErr err;
		
		// Add part information to the part object
		err = partObj.putString(Part::TYPE, GetPartTypeAsString(emailPart));
		ErrorToException(err);
		
		err = partObj.putString(Part::MIME_TYPE, emailPart->GetMimeType().c_str());
		ErrorToException(err);
		
		DatabaseAdapter::PutOptionalString(partObj, Part::PATH, emailPart->GetLocalFilePath());
		DatabaseAdapter::PutOptionalString(partObj, Part::NAME, emailPart->GetDisplayName());
		DatabaseAdapter::PutOptionalString(partObj, Part::CONTENT_ID, emailPart->GetContentId());
		DatabaseAdapter::PutOptionalString(partObj, Part::ENCODING, emailPart->GetEncoding());
		DatabaseAdapter::PutOptionalString(partObj, Part::CHARSET, emailPart->GetCharset());
		DatabaseAdapter::PutOptionalString(partObj, Part::SECTION, emailPart->GetSection());

		if(emailPart->GetEstimatedSize() >= 0) {
			err = partObj.put(Part::ESTIMATED_SIZE, emailPart->GetEstimatedSize());
			ErrorToException(err);
		}

		if(emailPart->GetEncodedSize() >= 0) {
			err = partObj.put(Part::ENCODED_SIZE, emailPart->GetEncodedSize());
			ErrorToException(err);
		}

		// Add to partsArrayObject
		err = partsArrayObj.push(partObj);
		ErrorToException(err);
	}
}
void EmailAdapter::SerializeFlags(const Email& email, MojObject& flags)
{
	MojErr err;

	err = flags.putBool(Flags::READ, email.IsRead());
	ErrorToException(err);

	err = flags.putBool(Flags::REPLIED, email.IsReplied());
	ErrorToException(err);

	err = flags.putBool(Flags::FLAGGED, email.IsFlagged());
	ErrorToException(err);
}

string EmailAdapter::GetHeaderPriority(Email::Priority priority) {
	switch (priority) {
	case Email::Priority_Highest:
		return HEADER_PRIORITY_HIGHEST;
	case Email::Priority_High:
		return HEADER_PRIORITY_HIGH;
	case Email::Priority_Normal:
		return HEADER_PRIORITY_NORMAL;
	case Email::Priority_Low:
		return HEADER_PRIORITY_LOW;
	case Email::Priority_Lowest:
		return HEADER_PRIORITY_LOWEST;
	default:
		return "";
	}
}

string EmailAdapter::GetHeaderImportance(Email::Priority priority) {
	if (priority == Email::Priority_Highest || priority == Email::Priority_High) {
		return HEADER_IMPORTANCE_HIGH;
	} else if(priority == Email::Priority_Lowest || priority == Email::Priority_Low) {
		return HEADER_IMPORTANCE_LOW;
	} else {
		return "";
	}
}
