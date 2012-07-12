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

#ifndef SYNCFOLDERLISTCOMMAND_H_
#define SYNCFOLDERLISTCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "db/MojDbClient.h"
#include "ImapCoreDefs.h"
#include "sync/FolderListDiff.h"
#include <boost/regex.hpp>
#include <vector>

class DeleteFoldersCommand;
class ListResponseParser;
class NamespaceResponseParser;

class SyncFolderListCommand : public ImapSessionCommand
{
	typedef boost::shared_ptr<ImapFolder> ImapFolderPtr;
	typedef boost::shared_ptr<Folder> FolderPtr;
	
public:
	SyncFolderListCommand(ImapSession& session);
	virtual ~SyncFolderListCommand();
	
	void RunImpl();
	
protected:
	void SendNamespaceCommand();
	MojErr NamespaceResponse();

	void GetLocalFolders();
	MojErr GetLocalFoldersResponse(MojObject& response, MojErr err);

	void SendListCommand();
	MojErr ListResponse();
	
	// Sync folders with database
	void ReconcileFolderList();
	
	void ReserveNewFolderIds();
	MojErr ReserveNewFolderIdsResponse(MojObject& response, MojErr err);

	void AssignFolderIds();

	void MatchParents();
	void CreateNewFolders();
	MojErr CreateNewFoldersResponse(MojObject& response, MojErr err);
	
	void DeleteFolders();
	MojErr DeleteFoldersDone();

	void UpdateAccount();
	MojErr UpdateAccountResponse(MojObject& response, MojErr err);
	
	void PickSpecialFolders();
	void UpdateSpecialFolders();

	void SetupWatchDraftsActivity();
	MojErr SetupWatchDraftsActivityResponse(MojObject& response, MojErr err);

	void Done();
	
	static inline bool IsValidId(const MojObject& obj) { return !obj.undefined() && !obj.null(); }
	
	MojRefCountedPtr<NamespaceResponseParser>	m_namespaceResponseParser;
	MojRefCountedPtr<ListResponseParser>		m_listResponseParser;

	std::string					m_namespacePrefix;

	std::vector<ImapFolderPtr>	m_localFolders;
	FolderListDiff				m_folderListDiff;
	bool						m_accountModified;
	MojDbQuery::Page			m_folderPage;
	
	std::vector<MojObject>		m_reservedIds;

	MojRefCountedPtr<DeleteFoldersCommand>	m_deleteFoldersCommand;

	MojSignal<>::Slot<SyncFolderListCommand>					m_namespaceResponseSlot;
	MojDbClient::Signal::Slot<SyncFolderListCommand>			m_getFoldersSlot;
	MojSignal<>::Slot<SyncFolderListCommand>					m_listResponseSlot;
	MojDbClient::Signal::Slot<SyncFolderListCommand>			m_reserveFolderIdsSlot;
	MojDbClient::Signal::Slot<SyncFolderListCommand>			m_createFoldersSlot;
	MojSignal<>::Slot<SyncFolderListCommand>					m_deleteFoldersSlot;
	MojDbClient::Signal::Slot<SyncFolderListCommand>			m_updateAccountSlot;
	MojDbClient::Signal::Slot<SyncFolderListCommand> 			m_setupWatchDraftsActivitySlot;

};

#endif /*SYNCFOLDERLISTCOMMAND_H_*/
