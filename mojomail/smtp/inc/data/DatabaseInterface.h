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

#ifndef DATABASEINTERFACE_H_
#define DATABASEINTERFACE_H_

#include "core/MojCoreDefs.h"
#include "core/MojServiceRequest.h"
#include "db/MojDbQuery.h"
#include <string>

class DatabaseInterface
{
public:
	typedef MojServiceRequest::ReplySignal Signal;

	virtual void GetAccount(Signal::SlotRef slot, const MojObject& accountId) = 0;

	virtual void GetMainAccount(Signal::SlotRef slot, const MojObject& accountId) = 0;

	virtual void GetOutboxEmail(Signal::SlotRef slot, const MojObject& emailId) = 0;

	virtual void GetOutboxEmails(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page) = 0;

	virtual void UpdateFolderRetry(Signal::SlotRef slot, const MojObject& folderId, const MojObject& retryDelay) = 0;

	virtual void UpdateAccountErrorStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& errorCode, const MojObject& errorText) = 0;

	//	virtual void GetEmails(Signal::SlotRef slot, const MojObject& folderId, int limit) = 0;
	
	virtual void DeleteItems			(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
                
	virtual void UpdateSendStatus		(Signal::SlotRef slot, const MojObject& emailId, const MojObject& status, const MojObject& visible) = 0;

	virtual void GetFolder				(Signal::SlotRef slot, const MojObject& accountId, const MojObject& folderId) = 0;

	virtual void PersistToDatabase		(Signal::SlotRef slot, MojObject& email, const MojObject& folderId, const MojObject& partsArray) = 0;

	virtual void PersistDraftToDatabase		(Signal::SlotRef slot, MojObject& email, const MojObject& folderId, const MojObject& partsArray) = 0;

	// for sync status

	virtual void CreateSyncStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& folderId, const char* state) = 0;
	virtual void ClearSyncStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& collectionId) = 0;
};

#endif /* DATABASEINTERFACE_H_ */
