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

#ifndef IMAPFOLDERADAPTER_H_
#define IMAPFOLDERADAPTER_H_

#include "data/ImapFolder.h"
#include "core/MojObject.h"

class ImapFolderAdapter
{
public:
	ImapFolderAdapter();
	virtual ~ImapFolderAdapter();
	
	static const char* const 	IMAP_FOLDER_KIND;
	static const char* const 	SERVER_FOLDER_NAME;
	static const char* const	DELIMITER;
	static const char* const	SELECTABLE;
	static const char* const	LAST_SYNC_REV;
	static const char* const	UIDVALIDITY;

	// Get data from MojoDB and turn them into email Folder
	static void		ParseDatabaseObject(const MojObject& obj, ImapFolder& folder);
	
	// Convert Folder data into MojoDB.
	static void		SerializeToDatabaseObject(const ImapFolder& folder, MojObject& obj);
};

#endif /*IMAPFOLDERADAPTER_H_*/
