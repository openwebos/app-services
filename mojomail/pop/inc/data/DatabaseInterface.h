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

	// Query Methods
	virtual void GetAccount				(Signal::SlotRef slot, const MojObject& accountId) = 0;
	virtual void GetMainAccount			(Signal::SlotRef slot, const MojObject& accountId) = 0;
	virtual void GetAccountFolders		(Signal::SlotRef slot, const MojObject& accountId) = 0;
	virtual void GetFolder				(Signal::SlotRef slot, const MojObject& folderId) = 0;
	virtual void GetEmail				(Signal::SlotRef slot, const MojObject& emailId) = 0;
	virtual void GetEmailTransportObj	(Signal::SlotRef slot, const MojObject& emailId) = 0;
	virtual void GetEmails				(Signal::SlotRef slot, const MojObject& folderId, MojInt32 limit) = 0;
	virtual void GetLocalEmailChanges	(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit) = 0;
	virtual void GetAutoDownloadEmails 	(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit) = 0;
	virtual void GetSentEmails			(Signal::SlotRef slot, const MojObject& outboxFolderId, MojInt32 limit = 0) = 0;
	virtual void GetDeletedEmails		(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit = 0) = 0;
	virtual void GetEmailSyncList		(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, bool desc, MojDbQuery::Page& page, MojInt32 limit = 0) = 0;
	virtual void GetUidCache			(Signal::SlotRef slot, const MojObject& accountId) = 0;
	virtual void GetEmailsToMove		(Signal::SlotRef slot, const MojObject& accountId) = 0;
	virtual void GetEmailsToDelete		(Signal::SlotRef slot) = 0;
	virtual void GetById				(Signal::SlotRef slot, const MojObject& id) = 0;
	virtual void GetByIds				(Signal::SlotRef slot, const MojObject::ObjectVec& ids) = 0;

	// Merge Methods
	virtual void UpdateEmailParts		(Signal::SlotRef slot, const MojObject& emailId, const MojObject& parts, bool forceDownloaded = false) = 0;
	virtual void UpdateEmailSummary		(Signal::SlotRef slot, const MojObject& emailId, const MojString& summary) = 0;
	virtual void UpdateAccount			(Signal::SlotRef slot, const MojObject& accountId, const MojObject& props) = 0;
	virtual void UpdateAccountFolders	(Signal::SlotRef slot, const MojObject& accountId, const MojObject& inboxFolderId,
										 const MojObject& draftsFolderId,
										 const MojObject& sentFolderId,
										 const MojObject& outboxFolderId,
										 const MojObject& trashFolderId) = 0;
	virtual void UpdateAccountRetry		(Signal::SlotRef slot, const MojObject& accountId, const MojObject& account) = 0;
	virtual void UpdateAccountInitialSync	(Signal::SlotRef slot, const MojObject& accountId, bool sync) = 0;
	virtual void MoveDeletedEmailToTrash (Signal::SlotRef slot, const MojObject& emailId, const MojObject& trashFolderId) = 0;

	// Sync Methods
	virtual void GetExistingItems		(Signal::SlotRef slot, const MojDbQuery& query) = 0;
	virtual void ReserveIds				(Signal::SlotRef slot, int count) = 0;
	virtual void AddItems				(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
	virtual void UpdateItem				(Signal::SlotRef slot, const MojObject& obj) = 0;
	virtual void UpdateItems			(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
	virtual void UpdateItems			(Signal::SlotRef slot, const MojDbQuery query, const MojObject values) = 0;
	virtual void UpdateItemRevisions	(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
	virtual void DeleteItems			(Signal::SlotRef slot, const MojObject::ObjectVec& array) = 0;
	virtual void DeleteItems			(Signal::SlotRef slot, const std::string kind, const std::string idField, const MojObject& id) = 0;

};

#endif /* DATABASEINTERFACE_H_ */
