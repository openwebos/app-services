// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef POPFOLDERADAPTER_H_
#define POPFOLDERADAPTER_H_

#include "PopDefs.h"
#include "core/MojObject.h"
#include "data/FolderAdapter.h"
#include "data/PopFolder.h"

class PopFolderAdapter : public FolderAdapter
{
public:
	static const char* const POP_FOLDER_KIND;
	static const char* const INBOX_FOLDER_ID;
	static const char* const OUTBOX_FOLDER_ID;
	static const char* const SENT_FOLDER_ID;
	static const char* const DRAFTS_FOLDER_ID;
	static const char* const TRASH_FOLDER_ID;
	static const char* const LAST_SYNC_REV;

	// Get data from MojoDB and turn them into email PopFolder
	static void	ParseDatabasePopObject(const MojObject& obj, PopFolder& folder);

	// Convert PopFolder data into MojoDB.
	static void	SerializeToDatabasePopObject(const PopFolder& folder, MojObject& obj);

	// Take ids from idArray and put into specialFolders with corresponding id name (e.g. "inboxFolderId":<inboxId>)
	// in the order:
	// Inbox, Outbox, Sent, Drafts, Trash
	static void SerializeSpecialFolders(const MojObject& idArray, MojObject& specialFolders);
};
#endif /* POPFOLDERADAPTER_H_ */
