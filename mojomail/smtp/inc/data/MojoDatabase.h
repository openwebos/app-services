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

#ifndef MOJODATABASE_H_
#define MOJODATABASE_H_

#include "data/DatabaseInterface.h"
#include "db/MojDbClient.h"

class MojoDatabase : public DatabaseInterface
{
public:
	MojoDatabase(MojDbClient& dbClient);
	virtual ~MojoDatabase();

	virtual void GetAccount(Signal::SlotRef slot, const MojObject& accountId);

	virtual void GetMainAccount(Signal::SlotRef slot, const MojObject& accountId);
	
	virtual void GetOutboxEmail(Signal::SlotRef slot, const MojObject& emailId);

	virtual void GetOutboxEmails(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page);

	virtual void UpdateFolderRetry(Signal::SlotRef slot, const MojObject& folderId, const MojObject& retryDelay);

	virtual void UpdateAccountErrorStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& errorCode, const MojObject& errorText);

	//	virtual void GetEmails(Signal::SlotRef slot, const MojObject& folderId, int limit);
	
	virtual void DeleteItems			(Signal::SlotRef slot, const MojObject::ObjectVec& array);

	// Outbox methods
	virtual void UpdateSendStatus		(Signal::SlotRef slot, const MojObject& emailId, const MojObject& status, const MojObject& visible);

	virtual void GetFolder				(Signal::SlotRef slot, const MojObject& accountId, const MojObject& folderId);

	virtual void PersistToDatabase		(Signal::SlotRef slot, MojObject& email, const MojObject& folderId, const MojObject& partsArray);

	virtual void PersistDraftToDatabase		(Signal::SlotRef slot, MojObject& email, const MojObject& folderId, const MojObject& partsArray);

	virtual void CreateSyncStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& folderId, const char* state);
	virtual void ClearSyncStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& collectionId);

private:
	/**
	 * Max number of IDs that can be reserved at one time.
	 */
	static const int RESERVE_ID_MAX = 1000;

	MojDbClient& m_dbClient;
};

#endif /* MOJODATABASE_H_ */
