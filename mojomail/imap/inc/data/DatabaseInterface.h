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

#ifndef DATABASEINTERFACE_H_
#define DATABASEINTERFACE_H_

#include "db/MojDbClient.h"
#include "data/ImapAccountAdapter.h"
#include "ImapCoreDefs.h"

class DatabaseInterface
{
public:
	typedef MojDbClient::Signal Signal;
	
	DatabaseInterface() {}
	virtual ~DatabaseInterface() {}
	
	// FIXME use more specific query methods
	virtual void Find(Signal::SlotRef slot, MojDbQuery query) = 0;
	virtual void GetById(Signal::SlotRef slot, const MojObject& id) = 0;
	virtual void GetByIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids) = 0;
	virtual void GetEmail(Signal::SlotRef slot, const MojObject& folderId, UID uid) = 0;

	virtual void CreateFolders(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
	virtual void ReserveIds(Signal::SlotRef slot, MojUInt32 num) = 0;
	virtual void DeleteIds(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
	virtual void PurgeIds(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
	virtual void DeleteFolderEmails(Signal::SlotRef slot, const MojObject& folderId) = 0;
	virtual void DeleteAccount(Signal::SlotRef slot, const MojObject& accountId) = 0;

	// Query Methods
	virtual void GetAccount(Signal::SlotRef slot, const MojObject& accountId) = 0;

	virtual void GetEmailChanges(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0) = 0;
	virtual void GetSentEmails(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0) = 0;
	virtual void GetDrafts(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0) = 0;
	virtual void GetMovedEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0) = 0;
	virtual void GetDeletedEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0) = 0;
	virtual void GetEmailSyncList(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0) = 0;

	virtual void GetAutoDownloads(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit) = 0;

	virtual void GetFolders(Signal::SlotRef slot, const MojObject& accountId, const MojDbQuery::Page& page, bool allFolders = false) = 0;

	// Currently used for getting the name of the trash folder, etc.
	virtual void GetFolderName(Signal::SlotRef slot, const MojObject& folderId) = 0;
	
	// Update methods
	virtual void UpdateAccount(Signal::SlotRef slot, const MojObject& accountId, const MojObject& props) = 0;
	virtual void UpdateAccountSpecialFolders(Signal::SlotRef slot, const ImapAccount& account) = 0;
	virtual void UpdateAccountError(Signal::SlotRef slot, const MojObject& accountId, const MojObject& errStatus) = 0;
	virtual void UpdateAccountRetry(Signal::SlotRef slot, const MojObject& accountId, const MojObject& retryStatus) = 0;

	virtual void PutEmails(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
	virtual void MergeFlags(Signal::SlotRef slot, const MojObject::ObjectVec& objects) = 0;
	virtual void UpdateEmails(Signal::SlotRef slot, const MojObject::ObjectVec& objects) = 0;
	virtual void DeleteEmailIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids) = 0;
	virtual void UpdateEmail(Signal::SlotRef slot, const MojObject& obj) = 0;
	virtual void UpdateFolder(Signal::SlotRef slot, const MojObject& obj) = 0;
};

#endif /*DATABASEINTERFACE_H_*/
