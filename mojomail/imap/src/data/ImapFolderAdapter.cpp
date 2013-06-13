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

#include "data/ImapFolderAdapter.h"
#include "data/FolderAdapter.h"
#include "ImapCoreDefs.h"
#include "data/DatabaseAdapter.h"

using namespace std;

ImapFolderAdapter::ImapFolderAdapter()
{
}

ImapFolderAdapter::~ImapFolderAdapter()
{
}

const char* const ImapFolderAdapter::IMAP_FOLDER_KIND 		= "com.palm.imap.folder:1";
const char* const ImapFolderAdapter::SERVER_FOLDER_NAME 	= "serverFolderName";
const char* const ImapFolderAdapter::DELIMITER			 	= "delimiter";
const char* const ImapFolderAdapter::SELECTABLE			 	= "selectable";
const char* const ImapFolderAdapter::LAST_SYNC_REV			= "lastSyncRev";
const char* const ImapFolderAdapter::UIDVALIDITY			= "uidValidity";

void ImapFolderAdapter::ParseDatabaseObject(const MojObject& obj, ImapFolder& folder)
{

	MojErr err;
	
	// Parse base Folder fields
	FolderAdapter::ParseDatabaseObject(obj, folder);

	MojString serverFolderName;
	err = obj.getRequired(SERVER_FOLDER_NAME, serverFolderName);
	ErrorToException(err);
	folder.SetFolderName(std::string(serverFolderName));
	
	string delimiter = DatabaseAdapter::GetOptionalString(obj, DELIMITER);
	folder.SetDelimiter(delimiter);

	bool hasUidValidity;
	UID uidValidity = 0;
	err = obj.get(UIDVALIDITY, uidValidity, hasUidValidity);
	ErrorToException(err);
	if(hasUidValidity) {
		folder.SetUIDValidity(uidValidity);
	}

	bool selectable = DatabaseAdapter::GetOptionalBool(obj, SELECTABLE, true);
	folder.SetSelectable(selectable);
}

void ImapFolderAdapter::SerializeToDatabaseObject(const ImapFolder& folder, MojObject& obj)
{
	// Serialize base Folder fields
	FolderAdapter::SerializeToDatabaseObject(folder, obj);
	
	MojErr err;
	// Overwrite the kind with the IMAP-specific folder kind
	err = obj.putString(FolderAdapter::KIND, IMAP_FOLDER_KIND);
	ErrorToException(err);
	
	// Set server folder name
	err = obj.putString(SERVER_FOLDER_NAME, folder.GetFolderName().c_str());
	ErrorToException(err);

	// Set delimiter string
	err = obj.putString(DELIMITER, folder.GetDelimiter().c_str());
	ErrorToException(err);

	// Selectable
	if(!folder.IsSelectable()) {
		err = obj.put(SELECTABLE, folder.IsSelectable());
		ErrorToException(err);
	}

	// UID validity
	if(folder.GetUIDValidity() > 0) {
		err = obj.put(UIDVALIDITY, (MojInt64) folder.GetUIDValidity());
		ErrorToException(err);
	}
}
