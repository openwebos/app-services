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

#ifndef MOJODATABASE_H_
#define MOJODATABASE_H_

#include "data/DatabaseInterface.h"

class MojoDatabase : public DatabaseInterface
{
public:
	MojoDatabase(MojDbClient& dbClient);
	virtual ~MojoDatabase();
	
	void Find(Signal::SlotRef slot, MojDbQuery query);
	void GetById(Signal::SlotRef slot, const MojObject& id);
	void GetByIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids);
	void GetEmail(Signal::SlotRef slot, const MojObject& folderId, UID uid);

	void CreateFolders(Signal::SlotRef slot, const MojObject::ObjectVec& array);
	void ReserveIds(Signal::SlotRef slot, MojUInt32 num);
	void DeleteIds(Signal::SlotRef slot, const MojObject::ObjectVec& array);
	void PurgeIds(Signal::SlotRef slot, const MojObject::ObjectVec& array);
	void DeleteFolderEmails(Signal::SlotRef slot, const MojObject& folderId);
	void DeleteAccount(Signal::SlotRef slot, const MojObject& accountId);
	
	void GetAccount(Signal::SlotRef slot, const MojObject& accountId);

	// Sync
	void GetEmailChanges(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0);
	void GetSentEmails(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0);
	void GetDrafts(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0);
	void GetMovedEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0);
	void GetDeletedEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0);
	void GetEmailSyncList(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0);

	void GetAutoDownloads(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit);

	void GetFolders(Signal::SlotRef slot, const MojObject& accountId, const MojDbQuery::Page& page, bool allFolders = false);
	void GetFolderName(Signal::SlotRef slot, const MojObject& folderId);
	
	void UpdateAccount(Signal::SlotRef slot, const MojObject& accountId, const MojObject& props);
	void UpdateAccountSpecialFolders(Signal::SlotRef slot, const ImapAccount& account);
	void UpdateAccountError(Signal::SlotRef slot, const MojObject& accountId, const MojObject& errStatus);
	void UpdateAccountRetry(Signal::SlotRef slot, const MojObject& accountId, const MojObject& retryStatus);

	void PutEmails(Signal::SlotRef slot, const MojObject::ObjectVec& ids);
	void MergeFlags(Signal::SlotRef slot, const MojObject::ObjectVec& objects);
	void UpdateEmails(Signal::SlotRef slot, const MojObject::ObjectVec& objects);
	void DeleteEmailIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids);
	
	void UpdateEmail(Signal::SlotRef slot, const MojObject& email);
	void UpdateFolder(Signal::SlotRef slot, const MojObject& obj);

protected:
	MojDbClient&	m_dbClient;
};

#endif /*MOJODATABASE_H_*/
