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

#include "SmtpClient.h"
#include "data/EmailAdapter.h"
#include "data/SmtpAccountAdapter.h"
#include "data/MojoDatabase.h"
#include "db/MojDbQuery.h"
#include "data/EmailSchema.h"
#include "data/DatabaseAdapter.h"
#include "data/SyncStateAdapter.h"

MojoDatabase::MojoDatabase(MojDbClient& dbClient)
: m_dbClient(dbClient)
{

}

MojoDatabase::~MojoDatabase()
{

}

void MojoDatabase::GetAccount(Signal::SlotRef slot, const MojObject& accountId)
{
	MojDbQuery q;

	MojErr err = q.from("com.palm.mail.account:1" /*SmtpAccountAdapter::SCHEMA*/);
	ErrorToException(err);

	err = q.where(SmtpAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	// now find the folder the e-mail belongs to
	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetMainAccount(Signal::SlotRef slot, const MojObject& accountId)
{
	MojDbQuery q;

	MojErr err = q.from("com.palm.account:1");
	ErrorToException(err);

	err = q.where(DatabaseAdapter::ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetOutboxEmail(Signal::SlotRef slot, const MojObject& emailId)
{
	MojDbQuery q;

	MojErr err = q.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	err = q.where("_id", MojDbQuery::OpEq, emailId);
	ErrorToException(err);

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetOutboxEmails(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page)
{
	MojDbQuery q;
	MojErr err = q.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	err = q.where("folderId", MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	err = q.order("_rev");
	ErrorToException(err);

	q.page(page);

	slot.cancel();

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::DeleteItems (Signal::SlotRef slot, const MojObject::ObjectVec& array)
{
	MojErr err = m_dbClient.del(slot, array.begin(), array.end());
	ErrorToException(err);
}

void MojoDatabase::UpdateSendStatus (Signal::SlotRef slot, const MojObject& emailId, const MojObject& status, const MojObject& visible)
{
	MojErr err;
	MojObject email;
	err = email.put(DatabaseAdapter::ID, emailId);
	ErrorToException(err);

	err = email.put(EmailSchema::SEND_STATUS, status);
	ErrorToException(err);

	if(!visible.undefined()) {
		err = email.put(EmailSchema::FLAGS, visible);
		ErrorToException(err);
	}

	err = m_dbClient.merge(slot, email);
	ErrorToException(err);
}

void MojoDatabase::GetFolder(Signal::SlotRef slot, const MojObject& accountId, const MojObject& folderId)
{
	MojErr err;
	MojDbQuery query;

	err = m_dbClient.get(slot, folderId);
	ErrorToException(err);
}

void MojoDatabase::UpdateFolderRetry(Signal::SlotRef slot, const MojObject& folderId, const MojObject& retryDelay)
{
	MojErr err;
	MojObject folder;
	err = folder.put(DatabaseAdapter::ID, folderId);
	ErrorToException(err);
	
	err = folder.put("smtpRetryDelay", retryDelay);
	ErrorToException(err);

	err = m_dbClient.merge(slot, folder);
	ErrorToException(err);
}


void MojoDatabase::UpdateAccountErrorStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& errorCode, const MojObject& errorText)
{
	MojErr err;
	MojDbQuery query;

	err = query.from("com.palm.mail.account:1"); // FIXME use a constant
	ErrorToException(err);

	err = query.where("accountId", MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	MojObject props;
	
	MojObject smtpConfig;
	
	MojObject error;	
	err = error.put("errorCode", errorCode);
	ErrorToException(err);
	
	err = error.put("errorText", errorText);
	ErrorToException(err);

	err = smtpConfig.put("error", error);
	ErrorToException(err);

	err = props.put("smtpConfig", smtpConfig);
	ErrorToException(err);

	err = m_dbClient.merge(slot, query, props, MojDb::FlagMerge);
	ErrorToException(err);
}

void MojoDatabase::PersistToDatabase(Signal::SlotRef slot,
									 MojObject& email,
										 const MojObject& folderId,
										 const MojObject& partsArray)
{
	MojErr err;

	// Set kind
	err = email.putString(EmailSchema::KIND, EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	// Replace folderId
	assert(!folderId.undefined());
	err = email.put(EmailSchema::FOLDER_ID, folderId);
	ErrorToException(err);

	// Replace parts array with modified version (with filecache paths, etc)
	err = email.put(EmailSchema::PARTS, partsArray);
	ErrorToException(err);

	// Add sendStatus
	MojObject sendStatus;
	err = sendStatus.put(EmailSchema::SendStatus::ERROR, MojObject::Null);
	ErrorToException(err);

	err = sendStatus.putBool(EmailSchema::SendStatus::FATAL_ERROR, false);
	ErrorToException(err);
	
	err = sendStatus.putInt(EmailSchema::SendStatus::RETRY_COUNT, 0);
	ErrorToException(err);
	
	err = email.put(EmailSchema::SEND_STATUS, sendStatus);
	ErrorToException(err);

	MojString folderIdJson;
	folderId.toJson(folderIdJson);
	//MojLogInfo(s_log, "saving email to %s folder %s", m_isDraft ? "drafts" : "outbox", folderIdJson.data());

	err = m_dbClient.put(slot, email);
	ErrorToException(err);
}

void MojoDatabase::PersistDraftToDatabase(Signal::SlotRef slot,
										 MojObject& email,
										 const MojObject& folderId,
										 const MojObject& partsArray)
{
	MojErr err;

	// Set kind
	err = email.putString(EmailSchema::KIND, EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	// Replace folderId
	assert(!folderId.undefined());
	err = email.put(EmailSchema::FOLDER_ID, folderId);
	ErrorToException(err);

	// Replace parts array with modified version (with filecache paths, etc)
	err = email.put(EmailSchema::PARTS, partsArray);
	ErrorToException(err);

	// Add sendStatus
	MojObject sendStatus;
	err = sendStatus.put(EmailSchema::SendStatus::ERROR, MojObject::Null);
	ErrorToException(err);

	err = sendStatus.putBool(EmailSchema::SendStatus::FATAL_ERROR, false);
	ErrorToException(err);

	err = sendStatus.putInt(EmailSchema::SendStatus::RETRY_COUNT, 0);
	ErrorToException(err);

	err = email.put(EmailSchema::SEND_STATUS, sendStatus);
	ErrorToException(err);

	MojString folderIdJson;
	folderId.toJson(folderIdJson);
	//MojLogInfo(s_log, "saving email to %s folder %s", m_isDraft ? "drafts" : "outbox", folderIdJson.data());

	err = m_dbClient.merge(slot, email);
	ErrorToException(err);
}

void MojoDatabase::CreateSyncStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& folderId, const char* state)
{
	MojObject syncStatus;

	MojErr err = syncStatus.putString(DatabaseAdapter::KIND, SyncStateAdapter::SYNC_STATE_KIND);
	ErrorToException(err);
	err = syncStatus.put(SyncStateAdapter::ACCOUNT_ID, accountId);
	ErrorToException(err);
	err = syncStatus.putString(SyncStateAdapter::CAPABILITY_PROVIDER, "com.palm.smtp.mail");
	ErrorToException(err);
	err = syncStatus.put(SyncStateAdapter::COLLECTION_ID, folderId);
	ErrorToException(err);
	err = syncStatus.putString(SyncStateAdapter::BUS_ADDRESS, "com.palm.smtp");
	ErrorToException(err);
	err = syncStatus.putString(SyncStateAdapter::SYNC_STATE, state);
	ErrorToException(err);

	m_dbClient.put(slot, syncStatus);
}

void MojoDatabase::ClearSyncStatus(Signal::SlotRef slot, const MojObject& accountId, const MojObject& collectionId)
{
	MojErr err;
	MojDbQuery query;

	err = query.from(SyncStateAdapter::SYNC_STATE_KIND);
	ErrorToException(err);
	err = query.where(SyncStateAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);
	err = query.where(SyncStateAdapter::COLLECTION_ID, MojDbQuery::OpEq, collectionId);
	ErrorToException(err);

	m_dbClient.del(slot, query);
}
