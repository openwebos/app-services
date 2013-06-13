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

#ifndef IMAPEMAILADAPTER_H_
#define IMAPEMAILADAPTER_H_

#include "core/MojObject.h"

class ImapEmail;
class EmailFlags;

class ImapEmailAdapter
{
public:
	ImapEmailAdapter();
	virtual ~ImapEmailAdapter();
	
	static const char* const 	IMAP_EMAIL_KIND;
	static const char* const 	UID;
	static const char* const	LAST_SYNC_FLAGS;
	static const char* const 	DEST_FOLDER_ID;
	static const char* const	AUTO_DOWNLOAD;
	static const char* const	UPSYNC_REV;
	
	// Get database object and read into an ImapEmail
	static void		ParseDatabaseObject(const MojObject& obj, ImapEmail& email);

	// Convert ImapEmail data into a database object
	static void		SerializeToDatabaseObject(const ImapEmail& folder, MojObject& obj);

	static void		ParseEmailFlags(const MojObject& flagsObj, EmailFlags& flags);
};

#endif /*IMAPEMAILADAPTER_H_*/
