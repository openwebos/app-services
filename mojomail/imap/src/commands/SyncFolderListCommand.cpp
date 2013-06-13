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

#include "commands/SyncFolderListCommand.h"
#include "client/ImapSession.h"
#include "data/ImapAccount.h"
#include "data/FolderAdapter.h"
#include "data/ImapAccountAdapter.h"
#include "data/ImapFolderAdapter.h"
#include "data/DatabaseInterface.h"
#include "data/DatabaseAdapter.h"
#include <map>
#include "ImapPrivate.h"
#include "sync/SpecialFolderPicker.h"
#include "activity/ActivityBuilder.h"
#include "activity/ImapActivityFactory.h"
#include "data/EmailSchema.h"
#include "commands/DeleteFoldersCommand.h"
#include "protocol/ListResponseParser.h"
#include "protocol/NamespaceResponseParser.h"
#include "client/Capabilities.h"
#include <boost/algorithm/string/predicate.hpp>

using namespace std;

SyncFolderListCommand::SyncFolderListCommand(ImapSession& session)
: ImapSessionCommand(session),
  m_accountModified(false),
  m_namespaceResponseSlot(this, &SyncFolderListCommand::NamespaceResponse),
  m_getFoldersSlot(this, &SyncFolderListCommand::GetLocalFoldersResponse),
  m_listResponseSlot(this, &SyncFolderListCommand::ListResponse),
  m_reserveFolderIdsSlot(this, &SyncFolderListCommand::ReserveNewFolderIdsResponse),
  m_createFoldersSlot(this, &SyncFolderListCommand::CreateNewFoldersResponse),
  m_deleteFoldersSlot(this, &SyncFolderListCommand::DeleteFoldersDone),
  m_updateAccountSlot(this, &SyncFolderListCommand::UpdateAccountResponse),
  m_setupWatchDraftsActivitySlot(this, &SyncFolderListCommand::SetupWatchDraftsActivityResponse)
{
}

SyncFolderListCommand::~SyncFolderListCommand()
{
}

void SyncFolderListCommand::RunImpl()
{
	// Get root folder prefix from account preferences
	m_namespacePrefix = m_session.GetAccount()->GetNamespaceRoot();

	// If there's no prefix, and the server supports NAMESPACE, run the NAMEPSACE command
	if(m_namespacePrefix.empty() && m_session.GetCapabilities().HasCapability(Capabilities::NAMESPACE)) {
		SendNamespaceCommand();
	} else {
		GetLocalFolders();
	}
}

void SyncFolderListCommand::SendNamespaceCommand()
{
	CommandTraceFunction();

	m_namespaceResponseParser.reset(new NamespaceResponseParser(m_session, m_namespaceResponseSlot));
	m_session.SendRequest("NAMESPACE", m_namespaceResponseParser);
}

MojErr SyncFolderListCommand::NamespaceResponse()
{
	CommandTraceFunction();

	try {
		m_namespaceResponseParser->CheckStatus();

		m_namespacePrefix = m_namespaceResponseParser->GetNamespacePrefix();

		GetLocalFolders();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncFolderListCommand::GetLocalFolders()
{
	CommandTraceFunction();

	assert( m_session.GetAccount().get() != NULL );
	const MojObject& accountId = m_session.GetAccount()->GetId();
	assert( !accountId.undefined() && !accountId.null() );

	m_session.GetDatabaseInterface().GetFolders(m_getFoldersSlot, accountId, m_folderPage);
}

MojErr SyncFolderListCommand::GetLocalFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	
	try {
		ErrorToException(err);
		
		MojObject accountId = m_session.GetAccount()->GetId();
		
		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);
		
		MojObject::ArrayIterator it;
		err = results.arrayBegin(it);
		ErrorToException(err);

		// Read list of local folders
		for (; it != results.arrayEnd(); ++it) {
			MojObject& folderObj = *it;
			ImapFolderPtr folder = boost::make_shared<ImapFolder>();
			
			ImapFolderAdapter::ParseDatabaseObject(folderObj, *folder);
			folder->SetAccountId(accountId);
			
			m_localFolders.push_back(folder);
		}
		
		if(DatabaseAdapter::GetNextPage(response, m_folderPage)) {
			// Get more folders
			GetLocalFolders();
		} else {
			// Next step: get list of emails from server
			SendListCommand();
		}
	} CATCH_AS_FAILURE
	
	return MojErrNone;
}

void SyncFolderListCommand::SendListCommand()
{
	CommandTraceFunction();
	
	m_listResponseParser.reset( new ListResponseParser(m_session, m_listResponseSlot) );
	
	bool useXlist = m_session.GetCapabilities().HasCapability(Capabilities::XLIST);

	stringstream ss;
	ss << (useXlist ? "XLIST " : "LIST ") << QuoteString(m_namespacePrefix) << " *";

	m_session.SendRequest(ss.str(), m_listResponseParser);
}

MojErr SyncFolderListCommand::ListResponse()
{
	CommandTraceFunction();
	
	try {
		m_listResponseParser->CheckStatus();

		MojLogInfo(m_log, "folders on server: %d", m_listResponseParser->GetFolders().size());

		ReconcileFolderList();
		
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncFolderListCommand::ReconcileFolderList()
{
	CommandTraceFunction();
	
	m_folderListDiff.GenerateDiff(m_localFolders, m_listResponseParser->GetFolders());

	if(m_folderListDiff.GetDeletedFolders().size() > 0) {
		// Delete new folders first
		DeleteFolders();
	} else if(m_folderListDiff.GetNewFolders().size() > 0) {
		// Create new folders first
		ReserveNewFolderIds();
	} else {
		// Just update the account
		UpdateAccount();
	}
}

void SyncFolderListCommand::PickSpecialFolders()
{
	const boost::shared_ptr<ImapAccount>& account = m_session.GetAccount();

	assert( account.get() != NULL );

	// Update special folders
	m_accountModified = false;
	SpecialFolderPicker folderPicker(m_folderListDiff.GetFolders());

	// Update Inbox
	if(!folderPicker.FolderIdExists(account->GetInboxFolderId())) {
		ImapFolderPtr inbox = folderPicker.MatchInbox();

		if(inbox.get()) {
			MojLogInfo(m_log, "setting inboxFolderId to folderId %s", AsJsonString(inbox->GetId()).c_str());

			account->SetInboxFolderId(inbox->GetId());
			m_accountModified = true;
		}
	}

	if(!folderPicker.FolderIdExists(account->GetArchiveFolderId())) {
		ImapFolderPtr archive = folderPicker.MatchArchive();

		if(archive.get()) {
			MojLogInfo(m_log, "setting archiveFolderId to folderId %s", AsJsonString(archive->GetId()).c_str());

			account->SetArchiveFolderId(archive->GetId());
			m_accountModified = true;
		} else {
			MojLogInfo(m_log, "setting archiveFolderId to null");

			account->SetArchiveFolderId(MojObject::Null);
			m_accountModified = true;
		}
	}

	if(!folderPicker.FolderIdExists(account->GetDraftsFolderId())) {
		ImapFolderPtr drafts = folderPicker.MatchDrafts();

		if(drafts.get()) {
			MojLogInfo(m_log, "setting draftsFolderId to folderId %s", AsJsonString(drafts->GetId()).c_str());

			account->SetDraftsFolderId(drafts->GetId());
			m_accountModified = true;
		} else {
			MojLogInfo(m_log, "setting draftsFolderId to null");

			account->SetDraftsFolderId(MojObject::Null);
			m_accountModified = true;
		}
	}

	if(!folderPicker.FolderIdExists(account->GetSentFolderId())) {
		ImapFolderPtr sent = folderPicker.MatchSent();

		if(sent.get()) {
			MojLogInfo(m_log, "setting sentFolderId to folderId %s", AsJsonString(sent->GetId()).c_str());

			account->SetSentFolderId(sent->GetId());
			m_accountModified = true;
		} else {
			MojLogInfo(m_log, "setting sentFolderId to null");

			account->SetSentFolderId(MojObject::Null);
			m_accountModified = true;
		}
	}

	if(!folderPicker.FolderIdExists(account->GetSpamFolderId())) {
		ImapFolderPtr spam = folderPicker.MatchSpam();

		if(spam.get()) {
			MojLogInfo(m_log, "setting spamFolderId to folderId %s", AsJsonString(spam->GetId()).c_str());

			account->SetSpamFolderId(spam->GetId());
			m_accountModified = true;
		} else {
			MojLogInfo(m_log, "setting spamFolderId to null");

			account->SetSpamFolderId(MojObject::Null);
			m_accountModified = true;
		}
	}

	if(!folderPicker.FolderIdExists(account->GetTrashFolderId())) {
		ImapFolderPtr trash = folderPicker.MatchTrash();

		if(trash.get()) {
			MojLogInfo(m_log, "setting trashFolderId to folderId %s", AsJsonString(trash->GetId()).c_str());

			account->SetTrashFolderId(trash->GetId());
			m_accountModified = true;
		} else {
			MojLogInfo(m_log, "setting trashFolderId to null");

			account->SetTrashFolderId(MojObject::Null);
			m_accountModified = true;
		}
	}
}

void SyncFolderListCommand::DeleteFolders()
{
	CommandTraceFunction();

	MojErr err;
	MojObject::ObjectVec deletedIds;

	MojLogInfo(m_log, "deleting %d folders", m_folderListDiff.GetDeletedFolders().size());
	// Get list of folder ids to delete
	BOOST_FOREACH(const ImapFolderPtr& folder, m_folderListDiff.GetDeletedFolders()) {
		if(boost::iequals(folder->GetFolderName(), ImapFolder::INBOX_FOLDER_NAME)) {
			MojLogError(m_log, "attempted to delete inbox!");
		} else {
			// Delete folder as long as it's not the inbox
			err = deletedIds.push(folder->GetId());
			ErrorToException(err);
		}
	}

	// DeleteFoldersCommands deletes all the specified folders, folders' emails, and watches.
	m_deleteFoldersCommand.reset(new DeleteFoldersCommand(*(m_session.GetClient()), deletedIds));
	m_deleteFoldersCommand->Run(m_deleteFoldersSlot);
}

MojErr SyncFolderListCommand::DeleteFoldersDone()
{
	CommandTraceFunction();

	try {
		m_deleteFoldersCommand->GetResult()->CheckException();

		if(m_folderListDiff.GetNewFolders().size()) {
			// Create new folders second
			ReserveNewFolderIds();
		} else {
			// No new folders; skip to updating account
			UpdateAccount();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncFolderListCommand::ReserveNewFolderIds()
{
	CommandTraceFunction();
	
	m_reserveFolderIdsSlot.cancel();
	m_session.GetDatabaseInterface().ReserveIds(m_reserveFolderIdsSlot, DatabaseAdapter::RESERVE_BATCH_SIZE);
}

MojErr SyncFolderListCommand::ReserveNewFolderIdsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	
	try {
		ErrorToException(err);
		
		const vector<ImapFolderPtr>& newFolders = m_folderListDiff.GetNewFolders();
		
		MojObject ids;
		err = response.getRequired("ids", ids);
		
		// Add ids
		BOOST_FOREACH(const MojObject& id, DatabaseAdapter::GetArrayIterators(ids)) {
			m_reservedIds.push_back(id);
		}
		
		if(m_reservedIds.size() < newFolders.size()) {
			if(ids.size() == 0)
				throw MailException("not enough ids reserved", __FILE__, __LINE__);

			ReserveNewFolderIds();
		} else {
			AssignFolderIds();
		}
	} CATCH_AS_FAILURE
	
	return MojErrNone;
}

void SyncFolderListCommand::AssignFolderIds()
{
	CommandTraceFunction();

	const vector<ImapFolderPtr>& newFolders = m_folderListDiff.GetNewFolders();

	// Assign ids to each new folder
	for(unsigned int i = 0; i < newFolders.size(); i++) {
		MojObject id = m_reservedIds.at(i);

		newFolders.at(i)->SetId(id);
	}

	CreateNewFolders();
}

// Associate folders with the right parent folders
// For each new folder, strip off the last component of the folder name.
// Look through new and existing folders for names that match.
void SyncFolderListCommand::MatchParents()
{
	const vector<ImapFolderPtr>& folders = m_folderListDiff.GetFolders();
	map<string, MojObject> suitableParents;
	
	vector<ImapFolderPtr>::const_iterator it;
	
	BOOST_FOREACH(const ImapFolderPtr& folder, folders) {
		// add to map
		assert( !folder->GetId().undefined() );
		suitableParents[folder->GetFolderName()] = folder->GetId();
	}
	
	// Match up parents
	BOOST_FOREACH(const ImapFolderPtr& folder, folders) {
		const string &folderName = folder->GetFolderName();
		size_t end = folderName.find_last_of(folder->GetDelimiter());
		
		if(end != string::npos) {
			std::string parentName = folderName.substr(0, end);

			// Fixup parent name to match our all-caps INBOX
			if(boost::istarts_with(parentName, ImapFolder::INBOX_FOLDER_NAME)) {
				parentName.replace(0, string(ImapFolder::INBOX_FOLDER_NAME).length(), ImapFolder::INBOX_FOLDER_NAME);
			}

			map<string, MojObject>::iterator parentIt = suitableParents.find(parentName);
			
			if(parentIt != suitableParents.end()) {
				MojObject parentId = parentIt->second;
				folder->SetParentId(parentId);
			}
		}
	}
}

void SyncFolderListCommand::CreateNewFolders()
{
	CommandTraceFunction();
	
	const vector<ImapFolderPtr>& newFolders = m_folderListDiff.GetNewFolders();
	MojLogInfo(m_log, "creating %d new folders", newFolders.size());
	
	MojObject::ObjectVec newFolderObjs;
	
	assert( m_session.GetAccount() != NULL );
	MojObject accountId = m_session.GetAccount()->GetId();

	// Pick special folders; note that ids have already been assigned at this point
	PickSpecialFolders();

	// Set parentId for new folders
	MatchParents();

	// Add folders (serialized into MojObjects) to array
	BOOST_FOREACH(const ImapFolderPtr& folder, newFolders) {
		MojObject folderObj;
		
		// Set account id first
		folder->SetAccountId(accountId);
		
		if(boost::iequals(folder->GetFolderName(), ImapFolder::INBOX_FOLDER_NAME)) {
			MojLogWarning(m_log, "attempted to re-add inbox folder '%s'; disallowing", folder->GetFolderName().c_str());
			continue;
		}

		// Set favorite flag on inbox
		if(m_session.GetAccount()->GetInboxFolderId() == folder->GetId()) {
			folder->SetFavorite(true);
		}

		ImapFolderAdapter::SerializeToDatabaseObject(*folder.get(), folderObj);
		newFolderObjs.push(folderObj);
	}

	m_session.GetDatabaseInterface().CreateFolders(m_createFoldersSlot, newFolderObjs);
}

MojErr SyncFolderListCommand::CreateNewFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	
	try {
		ErrorToException(err);
		
		UpdateAccount();
	} catch(const std::exception& e) {
		Failure(e);
	} catch(...) {
		// FIXME
	}
	return MojErrNone;
}

void SyncFolderListCommand::UpdateAccount()
{
	CommandTraceFunction();

	const boost::shared_ptr<ImapAccount>& account = m_session.GetAccount();
	assert( account.get() != NULL );

	// TODO: check for dangling folder ids?
	
	if(m_accountModified) {
		m_session.GetDatabaseInterface().UpdateAccountSpecialFolders(m_updateAccountSlot, *account);
	} else {
		Done();
	}
}

MojErr SyncFolderListCommand::UpdateAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
		
	try {
		ErrorToException(err);
		SetupWatchDraftsActivity();
	} catch(const std::exception& e) {
		Failure(e);
	} catch(...) {
		// FIXME
	}
	
	return MojErrNone;
}

void SyncFolderListCommand::SetupWatchDraftsActivity()
{
	MojObject accountId = m_session.GetAccount()->GetAccountId();
	MojObject folderId = m_session.GetAccount()->GetDraftsFolderId();

	ImapActivityFactory factory;
	ActivityBuilder actBuilder;

	factory.BuildDraftsWatch(actBuilder, accountId, folderId, 0);

	// create new activity (replacing any existing activity, if necessary)
	MojObject payload;
	MojErr err = payload.put("activity", actBuilder.GetActivityObject());
	ErrorToException(err);
	err = payload.put("start", true);
	ErrorToException(err);
	err = payload.put("replace", true);
	ErrorToException(err);

	m_session.GetClient()->SendRequest(m_setupWatchDraftsActivitySlot, "com.palm.activitymanager","create", payload);

}

MojErr SyncFolderListCommand::SetupWatchDraftsActivityResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);
		Done();
	} catch (const std::exception& e) {
		Failure(e);
	}

	return MojErrNone;
}

void SyncFolderListCommand::Done()
{
	CommandTraceFunction();
	m_session.FolderListSynced();
	Complete();
}
