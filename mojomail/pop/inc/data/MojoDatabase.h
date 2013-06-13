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

#ifndef MOJODATABASE_H_
#define MOJODATABASE_H_

#include "data/DatabaseInterface.h"
#include "db/MojDbClient.h"

class MojoDatabase : public DatabaseInterface
{
public:
	MojoDatabase(MojDbClient& dbClient);
	virtual ~MojoDatabase();

	// Query Methods
	virtual void GetAccount				(Signal::SlotRef slot, const MojObject& accountId);
	virtual void GetMainAccount			(Signal::SlotRef slot, const MojObject& accountId);
	virtual void GetAccountFolders		(Signal::SlotRef slot, const MojObject& accountId);
	virtual void GetFolder				(Signal::SlotRef slot, const MojObject& folderId);
	virtual void GetEmail				(Signal::SlotRef slot, const MojObject& emailId);
	virtual void GetEmailTransportObj	(Signal::SlotRef slot, const MojObject& emailId);
	virtual void GetEmails				(Signal::SlotRef slot, const MojObject& folderId, MojInt32 limit);
	virtual void GetLocalEmailChanges	(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit);
	virtual void GetAutoDownloadEmails	(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit);
	virtual void GetSentEmails			(Signal::SlotRef slot, const MojObject& outboxFolderId, MojInt32 limit = 0);
	virtual void GetDeletedEmails		(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit = 0);
	virtual void GetEmailSyncList		(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, bool desc, MojDbQuery::Page& page, MojInt32 limit = 0);
	virtual void GetUidCache			(Signal::SlotRef slot, const MojObject& accountId);
	virtual void GetEmailsToMove		(Signal::SlotRef slot, const MojObject& accountId);
	virtual void GetEmailsToDelete		(Signal::SlotRef slot);
	virtual void GetById				(Signal::SlotRef slot, const MojObject& id);
	virtual void GetByIds				(Signal::SlotRef slot, const MojObject::ObjectVec& ids);

	// Merge Methods
	virtual void UpdateEmailParts		(Signal::SlotRef slot, const MojObject& emailId, const MojObject& parts, bool skipAutoDownload = false);
	virtual void UpdateEmailSummary		(Signal::SlotRef slot, const MojObject& emailId, const MojString& summary);
	virtual void UpdateAccount			(Signal::SlotRef slot, const MojObject& accountId, const MojObject& props);
	virtual void UpdateAccountFolders	(Signal::SlotRef slot, const MojObject& accountId, const MojObject& inboxFolderId,
										 const MojObject& draftsFolderId,
										 const MojObject& sentFolderId,
										 const MojObject& outboxFolderId,
										 const MojObject& trashFolderId);
	virtual void UpdateAccountRetry		(Signal::SlotRef slot, const MojObject& accountId, const MojObject& account);
	virtual void UpdateAccountInitialSync	(Signal::SlotRef slot, const MojObject& accountId, bool sync);
	virtual void MoveDeletedEmailToTrash (Signal::SlotRef slot, const MojObject& emailId, const MojObject& trashFolderId);

	// Sync Methods
	virtual void GetExistingItems		(Signal::SlotRef slot, const MojDbQuery& query);
	virtual void ReserveIds				(Signal::SlotRef slot, int count);
	virtual void AddItems				(Signal::SlotRef slot, const MojObject::ObjectVec& array);
	virtual void UpdateItem				(Signal::SlotRef slot, const MojObject& obj);
	virtual void UpdateItems			(Signal::SlotRef slot, const MojObject::ObjectVec& array);
	virtual void UpdateItems			(Signal::SlotRef slot, const MojDbQuery query, const MojObject values);
	virtual void UpdateItemRevisions	(Signal::SlotRef slot, const MojObject::ObjectVec& array);
	virtual void DeleteItems			(Signal::SlotRef slot, const MojObject::ObjectVec& array);
	virtual void DeleteItems			(Signal::SlotRef slot, const std::string kind, const std::string idField, const MojObject& id);

private:
	/**
	 * Max number of IDs that can be reserved at one time.
	 */
	static const int RESERVE_ID_MAX = 1000;

	MojDbClient& 	m_dbClient;
};

#endif /* MOJODATABASE_H_ */
