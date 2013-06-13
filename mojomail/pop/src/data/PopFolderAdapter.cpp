// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "data/PopFolderAdapter.h"

const char* const PopFolderAdapter::POP_FOLDER_KIND 	= "com.palm.pop.folder:1";
const char* const PopFolderAdapter::INBOX_FOLDER_ID		= "inboxFolderId";
const char* const PopFolderAdapter::OUTBOX_FOLDER_ID	= "outboxFolderId";
const char* const PopFolderAdapter::SENT_FOLDER_ID		= "sentFolderId";
const char* const PopFolderAdapter::DRAFTS_FOLDER_ID	= "draftsFolderId";
const char* const PopFolderAdapter::TRASH_FOLDER_ID		= "trashFolderId";
const char* const PopFolderAdapter::LAST_SYNC_REV		= "lastSyncRev";

void PopFolderAdapter::ParseDatabasePopObject(const MojObject& obj, PopFolder& folder)
{
	// Get the command properties from MojObject
	ParseDatabaseObject(obj, folder);


	if(obj.contains(LAST_SYNC_REV)) {
		MojInt32 rev;
		MojErr err = obj.getRequired(LAST_SYNC_REV, rev);
		ErrorToException(err);
		folder.SetLastSyncRev(rev);
	}
}

void PopFolderAdapter::SerializeToDatabasePopObject(const PopFolder& folder, MojObject& obj)
{
	SerializeToDatabaseObject(folder, obj);

	MojErr err;
	// Set the object kind
	err = obj.putString(KIND, POP_FOLDER_KIND);
	ErrorToException(err);

	int lastSyncRev = folder.GetLastSyncRev();
	if (lastSyncRev > 0) {
		// only add this field if lastSyncRev value is greater than 0.
		// Since BaseSyncSession relies on the existance of this field to determine
		// whether it should start an account spinner or not for account creation,
		// we need to omit this field if it is an initial sync.
		err = obj.putInt(PopFolderAdapter::LAST_SYNC_REV, folder.GetLastSyncRev());
		ErrorToException(err);
	}
}

void PopFolderAdapter::SerializeSpecialFolders(const MojObject& idArray, MojObject& specialFolders)
{
	// add ids from idArray to specialFolders object in the order:
	// Inbox, Drafts, Outbox, Sent, Trash

	MojErr err;
	MojObject id;

	idArray.at(0, id);
	err = specialFolders.put(INBOX_FOLDER_ID, id);
	ErrorToException(err);

	idArray.at(1, id);
	err = specialFolders.put(DRAFTS_FOLDER_ID, id);
	ErrorToException(err);

	idArray.at(2, id);
	err = specialFolders.put(OUTBOX_FOLDER_ID, id);
	ErrorToException(err);

	idArray.at(3, id);
	err = specialFolders.put(SENT_FOLDER_ID, id);
	ErrorToException(err);

	idArray.at(4, id);
	err = specialFolders.put(TRASH_FOLDER_ID, id);
	ErrorToException(err);
}
