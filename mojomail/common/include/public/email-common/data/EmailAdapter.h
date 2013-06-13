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

#ifndef EMAILADAPTER_H_
#define EMAILADAPTER_H_

#include "data/CommonData.h"
#include "data/EmailPart.h"
#include "data/Email.h"

class EmailAdapter
{
public:
	static const char * const DRAFT_TYPE_NEW;
	static const char * const DRAFT_TYPE_REPLY;
	static const char * const DRAFT_TYPE_FORWARD;
	static const char * const PRIORITY_LOW;
	static const char * const PRIORITY_NORMAL;
	static const char * const PRIORITY_HIGH;
	static const char * const HEADER_IMPORTANCE_HIGH;
	static const char * const HEADER_IMPORTANCE_LOW;
	static const char * const HEADER_PRIORITY_HIGHEST;
	static const char * const HEADER_PRIORITY_HIGH;
	static const char * const HEADER_PRIORITY_NORMAL;
	static const char * const HEADER_PRIORITY_LOW;
	static const char * const HEADER_PRIORITY_LOWEST;

	// Parse a MojObject into an Email
	static void		ParseDatabaseObject(const MojObject& obj, Email& email);
	
	static void				ParseRecipients(const MojObject& recipients, Email& email);
	static Email::Priority 	ParsePriority(const MojString& priority);
	static Email::DraftType ParseDraftType(const MojString& draftType);
	static void				ParseFlags(const MojObject& flags, Email& email);
	static void				ParseParts(const MojObject& parts, EmailPartList& partList);
	
	// Serialize an entire Email object to the database
	static void			SerializeToDatabaseObject(const Email& email, MojObject& obj);
	
	static void			SerializeRecipients(const char* type, const EmailAddressListPtr& email, MojObject& recipients);
	static void			SerializeParts(const EmailPartList& partsList, MojObject& partsArrayObj);
	static void			SerializeFlags(const Email& email, MojObject& flags);
	static std::string 	GetHeaderPriority(Email::Priority priority);
	static std::string	GetHeaderImportance(Email::Priority priority);
	
protected:
	static EmailPart::PartType		GetPartType(const MojString& type);
	static const char*				GetPartTypeAsString(const EmailPartPtr& part);
	static EmailPartPtr				ParseEmailPart(const MojObject& partObj);
	static EmailAddressPtr			ParseAddress(const MojObject& addressObj);
	static void 					SerializeAddress(const char* type, EmailAddressPtr address, MojObject& recipient);
};

#endif /*EMAILADAPTER_H_*/
