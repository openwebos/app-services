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

#ifndef SYNCLOCALCHANGESCOMMAND_H_
#define SYNCLOCALCHANGESCOMMAND_H_

#include "commands/ImapSyncSessionCommand.h"
#include "db/MojDbClient.h"
#include "protocol/ImapResponseParser.h"
#include <string>
#include <vector>
#include <map>
#include <set>

class StoreResponseParser;
class MoveEmailsCommand;

class SyncLocalChangesCommand : public ImapSyncSessionCommand
{
public:
	SyncLocalChangesCommand(ImapSession& session, const MojObject& folderId);
	virtual ~SyncLocalChangesCommand();

	void RunImpl();
	void Status(MojObject& status) const;

protected:
	void GetChangedEmails();
	MojErr GetChangedEmailsResponse(MojObject& response, MojErr err);

	void SendRequests();
	void SendStoreRequest(MojRefCountedPtr<StoreResponseParser>& parser, const std::string& items, const std::vector<UID>& uids);
	MojErr StoreResponse();

	void MergeFlags();
	MojErr MergeFlagsResponse(MojObject& response, MojErr err);

	void FlagSyncComplete();

	void GetMovedEmails();
	MojErr GetMovedEmailsResponse(MojObject& response, MojErr err);

	void MoveEmails();
	MojErr MoveEmailsToFolderDone();
	void MoveComplete();

	void GetDeletedEmails();
	MojErr GetDeletedEmailsResponse(MojObject& response, MojErr err);

	void DeleteEmails();
	MojErr DeleteEmailsDone();

	void DeleteComplete();

	void Cleanup();

	bool		m_expunge;

	MojRefCountedPtr<StoreResponseParser>	m_storeResponseParser;
	MojRefCountedPtr<MoveEmailsCommand>		m_moveEmailsCommand;
	MojRefCountedPtr<MoveEmailsCommand>		m_moveDeleteEmailsCommand;

	MojDbQuery::Page		m_localChangesPage;
	MojDbQuery::Page		m_movedEmailsPage;
	MojDbQuery::Page		m_deletedEmailsPage;

	std::vector<UID> m_plusSeen, m_minusSeen;
	std::vector<UID> m_plusAnswered, m_minusAnswered;
	std::vector<UID> m_plusFlagged, m_minusFlagged;

	std::vector<UID>		m_uidsToDelete;

	// We use multimap to keep the 1:* mapping between
	// destination folder Id to mail Ids.
	std::multimap<MojObject, UID> m_folderIdUidMap;
	std::multimap<MojObject, MojObject> m_folderIdIdMap;

	// There could be multiple destination folders.
	// We need to keep a set of distinct folder IDs.
	std::set<MojObject> m_folderIdSet;

	// Local lastSyncFlags updates to reflect new status
	MojObject::ObjectVec	m_pendingMerge;

	// Local emails that need to be purged
	MojObject::ObjectVec	m_pendingDelete;

	// Local emails that need to be purged
	MojObject::ObjectVec	m_pendingMove;

	MojInt64				m_highestRevSeen;

	// Server name of the trash folder
	std::string				m_trashFolderName;

	MojDbClient::Signal::Slot<SyncLocalChangesCommand> m_getChangedEmailsSlot;
	MojDbClient::Signal::Slot<SyncLocalChangesCommand> m_mergeFlagsSlot;
	MojDbClient::Signal::Slot<SyncLocalChangesCommand> m_getDeletedEmailsSlot;
	MojDbClient::Signal::Slot<SyncLocalChangesCommand> m_getMovedEmailsSlot;

	ImapResponseParser::DoneSignal::Slot<SyncLocalChangesCommand>	m_storeResponseSlot;
	MojSignal<>::Slot<SyncLocalChangesCommand>						m_moveEmailsSlot;
	MojSignal<>::Slot<SyncLocalChangesCommand>						m_deleteEmailsSlot;
};

#endif /* SYNCLOCALCHANGESCOMMAND_H_ */
