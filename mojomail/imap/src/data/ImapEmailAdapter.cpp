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

#include "data/ImapEmailAdapter.h"
#include "data/ImapEmail.h"
#include "data/ImapFolder.h"
#include "data/EmailAdapter.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailSchema.h"
#include "ImapCoreDefs.h"

const char* const ImapEmailAdapter::IMAP_EMAIL_KIND 	= "com.palm.imap.email:1";
const char* const ImapEmailAdapter::UID 				= "uid";
const char* const ImapEmailAdapter::LAST_SYNC_FLAGS		= "lastSyncFlags";
const char* const ImapEmailAdapter::DEST_FOLDER_ID		= "destFolderId";
const char* const ImapEmailAdapter::AUTO_DOWNLOAD		= "autoDownload";
const char* const ImapEmailAdapter::UPSYNC_REV			= "UpsyncRev";

ImapEmailAdapter::ImapEmailAdapter()
{
}

ImapEmailAdapter::~ImapEmailAdapter()
{
}

void ImapEmailAdapter::ParseDatabaseObject(const MojObject& obj, ImapEmail& email)
{

	MojErr err;
	
	// Parse base Email fields
	EmailAdapter::ParseDatabaseObject(obj, email);

	::UID uid;
	err = obj.getRequired(UID, uid);
	ErrorToException(err);
	email.SetUID(uid);
}

void ImapEmailAdapter::SerializeToDatabaseObject(const ImapEmail& email, MojObject& obj)
{
	// Serialize base Folder fields
	EmailAdapter::SerializeToDatabaseObject(email, obj);
	
	MojErr err;
	// Overwrite the kind with the IMAP-specific email kind
	err = obj.putString(EmailSchema::KIND, IMAP_EMAIL_KIND);
	ErrorToException(err);
	
	// Set UID
	err = obj.put(UID, (MojInt64) email.GetUID());
	ErrorToException(err);

	// Copy last sync flags from the flags
	MojObject flagsObj;
	err = obj.getRequired(EmailSchema::FLAGS, flagsObj);
	ErrorToException(err);

	err = obj.put(LAST_SYNC_FLAGS, flagsObj);
	ErrorToException(err);

	// Set autodownload
	if(email.IsAutoDownload()) {
		err = obj.put(AUTO_DOWNLOAD, true);
		ErrorToException(err);
	}
}

void ImapEmailAdapter::ParseEmailFlags(const MojObject &flagsObj, EmailFlags& flags)
{
	flags.read = DatabaseAdapter::GetOptionalBool(flagsObj, EmailSchema::Flags::READ);
	flags.replied = DatabaseAdapter::GetOptionalBool(flagsObj, EmailSchema::Flags::REPLIED);
	flags.flagged = DatabaseAdapter::GetOptionalBool(flagsObj, EmailSchema::Flags::FLAGGED);
}
