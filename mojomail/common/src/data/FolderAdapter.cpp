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

#include "data/FolderAdapter.h"
#include "CommonPrivate.h"

const char* const FolderAdapter::FOLDER_KIND 			= "com.palm.folder:1";

const char* const FolderAdapter::KIND		 			= "_kind";
const char* const FolderAdapter::ID		 				= "_id";
const char* const FolderAdapter::ACCOUNT_ID 			= "accountId";
const char* const FolderAdapter::PARENT_ID	 			= "parentId";
const char* const FolderAdapter::DISPLAY_NAME 			= "displayName";
const char* const FolderAdapter::FAVORITE	 			= "favorite";
const char* const FolderAdapter::DEPTH		 			= "depth";

const char* const FolderAdapter::LAST_SYNC_TIME		 	= "lastSyncTime";
const char* const FolderAdapter::SYNC_STATUS 			= "syncStatus";

const char* const FolderAdapter::SYNC_STATUS_PUSH		= "push";
const char* const FolderAdapter::SYNC_STATUS_SCHEDULED	= "scheduled";
const char* const FolderAdapter::SYNC_STATUS_MANUAL		= "manual";
const char* const FolderAdapter::SYNC_STATUS_SYNCING	= "syncing";

void FolderAdapter::ParseDatabaseObject(const MojObject& obj, Folder& folder)
{

	MojErr err;
	
	if(obj.contains(ID)) {
		MojObject id;
		err = obj.getRequired(ID, id);
		ErrorToException(err);
		folder.SetId(id);
	}

	MojObject accountId;
	err = obj.getRequired(ACCOUNT_ID, accountId);
	ErrorToException(err);
	folder.SetAccountId(accountId);
	
	// Optional parentId -- default to null (no parent)
	MojObject parentId;
	if(obj.get(PARENT_ID, parentId)) {
		ErrorToException(err);
		folder.SetParentId(parentId);
	}

	MojString displayName;
	err = obj.getRequired(DISPLAY_NAME, displayName);
	ErrorToException(err);
	folder.SetDisplayName(std::string(displayName));
	
	// Optional favorite flag -- defaults to false
	bool favorite = false;
	if(obj.get(FAVORITE, favorite)) {
		ErrorToException(err);
		folder.SetFavorite(favorite);
	}
}

void FolderAdapter::SerializeToDatabaseObject(const Folder& folder, MojObject& obj)
{
	MojErr err;
	// Set the object kind
	err = obj.putString(KIND, FOLDER_KIND);
	ErrorToException(err);
	
	if(!folder.GetId().undefined()) {
		err = obj.put(ID, folder.GetId());
		ErrorToException(err);
	}

	// Set account id
	err = obj.put(ACCOUNT_ID, folder.GetAccountId());
	ErrorToException(err);
	
	// Set parent folder id
	err = obj.put(PARENT_ID, folder.GetParentId());
	ErrorToException(err);

	// Set display name
	err = obj.putString(DISPLAY_NAME, folder.GetDisplayName().c_str());
	ErrorToException(err);

	// Set favorite flag
	err = obj.putBool(FAVORITE, folder.GetFavorite());
	ErrorToException(err);
}
