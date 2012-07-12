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

#include "commands/CreateDefaultFoldersCommand.h"
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
#include "activity/ImapActivityFactory.h"
#include "activity/ActivityBuilder.h"
#include "data/EmailSchema.h"
#include "data/EmailAccountAdapter.h"

using namespace std;

CreateDefaultFoldersCommand::CreateDefaultFoldersCommand(ImapClient& client)
: ImapClientCommand(client),
  m_needsInbox(false),
  m_needsOutbox(false),
  m_getFoldersSlot(this, &CreateDefaultFoldersCommand::FindSpecialFoldersResponse),
  m_createFoldersSlot(this, &CreateDefaultFoldersCommand::CreateFoldersResponse),
  m_updateAccountSlot(this, &CreateDefaultFoldersCommand::UpdateAccountResponse),
  m_setupOutboxWatchActivitySlot(this, &CreateDefaultFoldersCommand::SetupOutboxWatchActivityResponse)
{
}

CreateDefaultFoldersCommand::~CreateDefaultFoldersCommand()
{
}

void CreateDefaultFoldersCommand::RunImpl()
{
	MojLogInfo(m_log, "creating default folders");

	m_accountId = m_client.GetAccountId();

	try {
		FindSpecialFolders();
	} CATCH_AS_FAILURE
}

// Find existing special folders
void CreateDefaultFoldersCommand::FindSpecialFolders()
{
	CommandTraceFunction();

	MojObject::ObjectVec folders;

	EmailAccountAdapter::AppendSpecialFolderIds(m_client.GetAccount(), folders);

	m_client.GetDatabaseInterface().GetByIds(m_getFoldersSlot, folders);
}

MojErr CreateDefaultFoldersCommand::FindSpecialFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	MojObject existingFolders;

	ImapAccount& account = m_client.GetAccount();

	// Get ids from database response
	MojObject::ObjectVec folderIds;

	// Match up ids
	BOOST_FOREACH(const MojObject& folder, DatabaseAdapter::GetResultsIterators(response)) {
		MojObject id;

		err = folder.getRequired(DatabaseAdapter::ID, id);
		ErrorToException(err);

		folderIds.push(id);
	}

	// Clear out bad folder ids
	EmailAccountAdapter::ClearMissingFolderIds(account, folderIds);

	CreateFolders();

	return MojErrNone;
}

void CreateDefaultFoldersCommand::CreateFolders()
{
	CommandTraceFunction();

	MojErr err;
	MojObject::ObjectVec newFolderObjs;

	// Create inbox if an inbox doesn't exist
	if(!IsValidId(m_client.GetAccount().GetInboxFolderId())) {
		MojLogInfo(m_log, "creating new inbox for account %s", AsJsonString(m_accountId).c_str());

		ImapFolder inboxFolder;
		MojObject inboxObj;

		inboxFolder.SetDisplayName("Inbox");  //TODO: Use a constant string.
		inboxFolder.SetFolderName("INBOX");
		inboxFolder.SetAccountId(m_accountId);
		inboxFolder.SetFavorite(true);

		ImapFolderAdapter::SerializeToDatabaseObject(inboxFolder, inboxObj);
		err = newFolderObjs.push(inboxObj);
		ErrorToException(err);

		m_needsInbox = true;
	}

	// Create outbox if an outbox doesn't exist
	if(!IsValidId(m_client.GetAccount().GetOutboxFolderId())) {
		MojLogInfo(m_log, "creating new outbox for account %s", AsJsonString(m_accountId).c_str());

		Folder outboxFolder;
		MojObject outboxObj;

		outboxFolder.SetDisplayName("Outbox");  //TODO: Use a constant string.
		outboxFolder.SetAccountId(m_accountId);

		FolderAdapter::SerializeToDatabaseObject(outboxFolder, outboxObj);
		err = newFolderObjs.push(outboxObj);
		ErrorToException(err);

		m_needsOutbox = true;
	}

	if(!newFolderObjs.empty()) {
		// The order of the folder objects must be inbox first and outbox second.
		m_client.GetDatabaseInterface().CreateFolders(m_createFoldersSlot, newFolderObjs);
	} else {
		// No new folders; just update the account
		UpdateAccount();
	}
}

MojErr CreateDefaultFoldersCommand::CreateFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);

		MojLogInfo(m_log, "create folders result: %s", AsJsonString(results).c_str());

		int index = 0;

		// Get new inbox folder id
		if(m_needsInbox) {
			MojObject inboxResponseObj;
			if (results.at(index, inboxResponseObj)) {
				MojObject inboxId;

				err = inboxResponseObj.getRequired(DatabaseAdapter::RESULT_ID, inboxId);
				ErrorToException(err);

				m_client.GetAccount().SetInboxFolderId(inboxId);
			} else {
				throw MailException("error creating inbox", __FILE__, __LINE__);
			}

			index++;
		}

		// Get new outbox folder id
		if(m_needsOutbox) {
			MojObject outboxResponseObj;
			if(results.at(index, outboxResponseObj)) {
				MojObject outboxId;

				err = outboxResponseObj.getRequired(DatabaseAdapter::RESULT_ID, outboxId);
				ErrorToException(err);

				m_client.GetAccount().SetOutboxFolderId(outboxId);
			} else {
				throw MailException("error creating outbox", __FILE__, __LINE__);
			}

			index++;
		}
		
		UpdateAccount();
	} CATCH_AS_FAILURE

	return MojErrNone;
}


void CreateDefaultFoldersCommand::UpdateAccount()
{
	CommandTraceFunction();

	ImapAccount& account = m_client.GetAccount();

	MojErr err;
	MojObject toMerge;

	// Update special folders
	EmailAccountAdapter::SerializeSpecialFolders(account, toMerge);

	// Clear error, if any
	if(account.GetError().errorCode != MailError::NONE) {
		MojObject errorObj;

		account.ClearError();
		EmailAccountAdapter::SerializeErrorInfo(account.GetError(), errorObj);

		err = toMerge.put(EmailAccountAdapter::ERROR, errorObj);
		ErrorToException(err);
	}

	m_client.GetDatabaseInterface().UpdateAccount(m_updateAccountSlot, m_accountId, toMerge);
}

MojErr CreateDefaultFoldersCommand::UpdateAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		SetupOutboxWatchActivity();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void CreateDefaultFoldersCommand::SetupOutboxWatchActivity()
{
	CommandTraceFunction();

	MojErr err;

	ImapActivityFactory factory;
	ActivityBuilder actBuilder;

	factory.BuildOutboxWatch(actBuilder, m_accountId, m_client.GetAccount().GetOutboxFolderId(), 0);

	MojObject payload;
	err = payload.put("activity", actBuilder.GetActivityObject());
	ErrorToException(err);
	err = payload.put("start", true);
	ErrorToException(err);
	err = payload.put("replace", true);
	ErrorToException(err);

	m_client.SendRequest(m_setupOutboxWatchActivitySlot, "com.palm.activitymanager", "create", payload);
}

MojErr CreateDefaultFoldersCommand::SetupOutboxWatchActivityResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		Done();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void CreateDefaultFoldersCommand::Done()
{
	CommandTraceFunction();

	Complete();
}
