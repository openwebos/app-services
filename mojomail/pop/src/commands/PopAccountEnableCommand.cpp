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

#include "commands/PopAccountEnableCommand.h"
#include "data/DatabaseAdapter.h"
#include "activity/ActivityBuilder.h"

PopAccountEnableCommand::PopAccountEnableCommand(PopClient& client, MojServiceMessage* msg, MojObject& payload)
: PopClientCommand(client, "Enable POP account", Command::HighPriority),
  m_msg(msg),
  m_payload(payload),
  m_needsInbox(false),
  m_needsDrafts(false),
  m_needsOutbox(false),
  m_needsSentFolder(false),
  m_needsTrashFolder(false),
  m_needsInitialSync(false),
  m_capabilityProvider("com.palm.pop.mail"),
  m_busAddress("com.palm.pop"),
  m_updateAccountSlot(this, &PopAccountEnableCommand::UpdateAccountResponse),
  m_getFoldersSlot(this, &PopAccountEnableCommand::FindSpecialFoldersResponse),
  m_createSlot(this, &PopAccountEnableCommand::CreatePopFoldersResponse),
  m_getMailAccountSlot(this, &PopAccountEnableCommand::GetMailAccountResponse),
  m_createOutboxWatchSlot(this, &PopAccountEnableCommand::CreateOutboxWatchResponse),
  m_createMoveEmailWatchSlot(this, &PopAccountEnableCommand::CreateMoveEmailWatchResponse),
  m_smtpAccountEnabledSlot(this, &PopAccountEnableCommand::SmtpAccountEnabledResponse),
  m_updateMissingCredentialsResponseSlot(this, &PopAccountEnableCommand::UpdateMissingCredentialsSyncStatusResponse)
{

}

PopAccountEnableCommand::~PopAccountEnableCommand()
{

}

void PopAccountEnableCommand::RunImpl()
{
	try {
		if (!m_client.GetAccount().get()) {
			MojString err;
			err.format("Account is not loaded for '%s'", AsJsonString(
					m_client.GetAccountId()).c_str());
			throw MailException(err.data(), __FILE__, __LINE__);
		}

		m_account = m_client.GetAccount();
		m_accountId = m_account->GetAccountId();
		FindSpecialFolders();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception in enabling POP account", __FILE__, __LINE__));
	}
}

// Find existing special folders
void PopAccountEnableCommand::FindSpecialFolders()
{
	CommandTraceFunction();

	MojObject::ObjectVec folders;

	EmailAccountAdapter::AppendSpecialFolderIds(*m_account.get(), folders);

	m_client.GetDatabaseInterface().GetByIds(m_getFoldersSlot, folders);
}


MojErr PopAccountEnableCommand::FindSpecialFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	PopAccount& account = *m_account.get();

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

	CreatePopFolders();

	return MojErrNone;
}

void PopAccountEnableCommand::AddPopFolder(MojObject::ObjectVec& folders,
									  const char* displayName, bool favorite)
{
	MojLogTrace(m_log);

	MojObject id;

	PopFolder folder;
	folder.SetAccountId(m_accountId);
	folder.SetDisplayName(displayName);
	folder.SetFavorite(favorite);
	folder.SetId(id);

	MojObject mojFolderObj;
	PopFolderAdapter::SerializeToDatabasePopObject(folder, mojFolderObj);
	folders.push(mojFolderObj);

	MojString temp;
	mojFolderObj.toJson(temp);
	MojLogDebug(m_log, "AddPopFolder: %s\n", temp.data());
}

void PopAccountEnableCommand::CreatePopFolders()
{
	CommandTraceFunction();

	MojObject::ObjectVec folders;

	// Create inbox if an inbox doesn't exist
	if(!IsValidId(m_account->GetInboxFolderId())) {
		MojLogInfo(m_log, "creating new inbox for account %s\n", AsJsonString(m_accountId).c_str());
		AddPopFolder(folders, "Inbox", true);
		m_needsInitialSync = true;
		MojLogInfo(m_log, "inital sync for account %s\n", m_needsInitialSync?"TRUE":"FALSE");
		m_account->SetInitialSync(m_needsInitialSync);
		m_needsInbox = true;
	}

	// Create drafts if an drafts doesn't exist
	if(!IsValidId(m_account->GetDraftsFolderId())) {
		MojLogInfo(m_log, "creating new drafts for account %s\n", AsJsonString(m_accountId).c_str());
		AddPopFolder(folders, "Drafts", false);
		m_needsDrafts = true;
	}

	// Create outbox if an outbox doesn't exist
	if(!IsValidId(m_account->GetOutboxFolderId())) {
		MojLogInfo(m_log, "creating new outbox for account %s\n", AsJsonString(m_accountId).c_str());
		AddPopFolder(folders, "Outbox", false);
		m_needsOutbox = true;
	}

	// Create sent folder if an sent folder doesn't exist
	if(!IsValidId(m_account->GetSentFolderId())) {
		MojLogInfo(m_log, "creating new sent folder for account %s\n", AsJsonString(m_accountId).c_str());
		AddPopFolder(folders, "Sent", false);
		m_needsSentFolder = true;
	}

	// Create trash if an trash doesn't exist
	if(!IsValidId(m_account->GetTrashFolderId())) {
		MojLogInfo(m_log, "creating new trash folder for account %s\n", AsJsonString(m_accountId).c_str());
		AddPopFolder(folders, "Trash", false);
		m_needsTrashFolder = true;
	}

	if(!folders.empty()) {
		// The order of the folder objects must be
		// inbox, drafts, outbox, sent folder, trash folder.
		m_client.GetDatabaseInterface().AddItems(m_createSlot, folders);
	} else {
		// No new folders; just update the account
		UpdateAccount();
	}
}

MojErr PopAccountEnableCommand::CreatePopFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		MojObject inboxId;
		MojObject draftsId;
		MojObject outboxId;
		MojObject sentFolderId;
		MojObject trashFolderId;

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);

		MojLogInfo(m_log, "create folders result: %s\n", AsJsonString(results).c_str());

		int index = 0;

		// Get new inbox folder id
		if(m_needsInbox) {
			MojObject inboxResponseObj;
			if (results.at(index, inboxResponseObj)) {
				err = inboxResponseObj.getRequired(DatabaseAdapter::RESULT_ID, inboxId);
				ErrorToException(err);
				m_account->SetInboxFolderId(inboxId);
			} else {
				throw MailException("error creating inbox", __FILE__, __LINE__);
			}

			index++;
		}

		// Get new drafts folder id
		if(m_needsDrafts) {
			MojObject draftsResponseObj;
			if(results.at(index, draftsResponseObj)) {
				err = draftsResponseObj.getRequired(DatabaseAdapter::RESULT_ID, draftsId);
				ErrorToException(err);
				m_account->SetDraftsFolderId(draftsId);
			} else {
				throw MailException("error creating drafts", __FILE__, __LINE__);
			}

			index++;
		}

		// Get new outbox id
		if(m_needsOutbox) {
			MojObject outboxResponseObj;
			if(results.at(index, outboxResponseObj)) {
				err = outboxResponseObj.getRequired(DatabaseAdapter::RESULT_ID, outboxId);
				ErrorToException(err);
				m_account->SetOutboxFolderId(outboxId);
			} else {
				throw MailException("error creating outbox", __FILE__, __LINE__);
			}

			index++;
		}

		// Get new sent folder id
		if(m_needsSentFolder) {
			MojObject sentFolderResponseObj;
			if(results.at(index, sentFolderResponseObj)) {
				err = sentFolderResponseObj.getRequired(DatabaseAdapter::RESULT_ID, sentFolderId);
				ErrorToException(err);
				m_account->SetSentFolderId(sentFolderId);
			} else {
				throw MailException("error creating sent folder", __FILE__, __LINE__);
			}

			index++;
		}

		// Get new trash folder id
		if(m_needsTrashFolder) {
			MojObject trashFolderResponseObj;
			if(results.at(index, trashFolderResponseObj)) {
				err = trashFolderResponseObj.getRequired(DatabaseAdapter::RESULT_ID, trashFolderId);
				ErrorToException(err);
				m_account->SetTrashFolderId(trashFolderId);
			} else {
				throw MailException("error creating trash folder", __FILE__, __LINE__);
			}

			index++;
		}

		UpdateAccount();

	} CATCH_AS_FAILURE

	return MojErrNone;
}



void PopAccountEnableCommand::UpdateAccount() {

	PopAccount& account = *m_account.get();

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

	err = toMerge.putBool(PopAccountAdapter::INITIAL_SYNC, m_needsInitialSync);
	ErrorToException(err);
	m_client.GetDatabaseInterface().UpdateAccount(m_updateAccountSlot, m_accountId, toMerge);

}

MojErr PopAccountEnableCommand::UpdateAccountResponse(MojObject& response, MojErr err)
{
	try {
		// check error response
		ErrorToException(err);

		MojLogTrace(m_log);
		MojLogInfo(m_log, "SyncAccountCommand::SetSpecialFoldersOnAccountResponse: %s\n", AsJsonString(response).c_str());

		// create watch on POP outbox
		CreateOutboxWatch();
	} catch (const std::exception& ex) {
		m_msg->replyError(err, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountEnableCommand::CreateOutboxWatch()
{
	m_client.GetDatabaseInterface().GetAccount(m_getMailAccountSlot, m_accountId);
}

MojErr PopAccountEnableCommand::GetMailAccountResponse(MojObject& response, MojErr err)
{

	try {
		ErrorToException(err);

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);

		if (results.size() > 0 && results.at(0, m_transportObj)) {
			CreateOutboxWatchActivity();
		}
	} catch (const std::exception& ex) {
		m_msg->replyError(err, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountEnableCommand::CreateOutboxWatchActivity()
{

	// TODO: define _rev, accountId, outboxFolderId somewhere and use those constants instead
	// 		 of the raw string

	// get outboxFolderId
	MojObject outboxFolderId;
	MojErr err = m_transportObj.getRequired(PopFolderAdapter::OUTBOX_FOLDER_ID, outboxFolderId);
	ErrorToException(err);

	// get sentFolderId
	MojObject sentFolderId;
	err = m_transportObj.getRequired(PopFolderAdapter::SENT_FOLDER_ID, sentFolderId);
	ErrorToException(err);

	// activity to setup watch
	ActivityBuilder ab;
	m_client.GetActivityBuilderFactory()->BuildSentEmailsWatch(ab, outboxFolderId, sentFolderId);

	// create new activity (replacing any existing activity if necessary)
	MojObject payload;
	err = payload.put("activity", ab.GetActivityObject());
	ErrorToException(err);
	err = payload.put("start", true);
	ErrorToException(err);
	err = payload.put("replace", true);
	ErrorToException(err);

	m_client.SendRequest(m_createOutboxWatchSlot, "com.palm.activitymanager", "create", payload);
}

MojErr PopAccountEnableCommand::CreateOutboxWatchResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojString json;
		err = response.toJson(json);
		ErrorToException(err);
		MojLogInfo(m_log, "response from activitymanager re: account watch: %s", json.data());

		CreateMoveEmailWatch();
	} catch (const std::exception& ex) {
		m_msg->replyError(err, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountEnableCommand::CreateMoveEmailWatch()
{
	// activity to setup watch
	ActivityBuilder ab;
	m_client.GetActivityBuilderFactory()->BuildMoveEmailsWatch(ab);

	// create new activity (replacing any existing activity if necessary)
	MojObject payload;
	MojErr err = payload.put("activity", ab.GetActivityObject());
	ErrorToException(err);
	err = payload.put("start", true);
	ErrorToException(err);
	err = payload.put("replace", true);
	ErrorToException(err);

	m_client.SendRequest(m_createMoveEmailWatchSlot, "com.palm.activitymanager", "create", payload);
}

MojErr PopAccountEnableCommand::CreateMoveEmailWatchResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojLogInfo(m_log, "response from activitymanager re: move email watch: %s", AsJsonString(response).c_str());

		EnableSmtpAccount();

	} catch (const std::exception& ex) {
		m_msg->replyError(err, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountEnableCommand::EnableSmtpAccount()
{
	m_client.SendRequest(m_smtpAccountEnabledSlot, "com.palm.smtp", "accountEnabled", m_payload);
}

MojErr PopAccountEnableCommand::SmtpAccountEnabledResponse(MojObject& response, MojErr err)
{
	try {
		// check error response
		ErrorToException(err);
		UpdateMissingCredentialsSyncStatus();

	} catch (const std::exception& ex) {
		m_msg->replyError(err, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountEnableCommand::UpdateMissingCredentialsSyncStatus()
{
	CommandTraceFunction();

	if(!m_client.GetAccount()->HasPassword())
	{
		MojLogError(m_log, "EnableAccountCommand::UpdateMissingCredentialsSyncStatus(): no password!\n");
		MojObject accountId = m_client.GetAccountId();
		SyncState syncState(accountId);
		syncState.SetState(SyncState::ERROR);
		syncState.SetError("CREDENTIALS_NOT_FOUND", "error: credentials not found!");
		m_syncStateUpdater.reset(new SyncStateUpdater(m_client, m_capabilityProvider.data(), m_busAddress.data()));
		m_syncStateUpdater->UpdateSyncState(m_updateMissingCredentialsResponseSlot, syncState);
	} else {
		MojLogError(m_log, "EnableAccountCommand::UpdateMissingCredentialsSyncStatus(): has password!\n");
		Done();
	}
}

MojErr PopAccountEnableCommand::UpdateMissingCredentialsSyncStatusResponse()
{
	MojLogError(m_log, "EnableAccountCommand::UpdateMissingCredentialsSyncStatusResponse(): sync status updated\n");
	Done();
	return MojErrNone;
}

void PopAccountEnableCommand::Done()
{
	m_msg->replySuccess();
	Complete();
}
