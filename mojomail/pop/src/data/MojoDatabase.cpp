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

#include "data/MojoDatabase.h"
#include "data/UidCacheAdapter.h"
#include "data/PopAccountAdapter.h"
#include "data/PopEmailAdapter.h"
#include "data/PopFolderAdapter.h"
#include "data/EmailSchema.h"
#include "db/MojDbQuery.h"
#include "PopClient.h"
#include "data/DatabaseAdapter.h"

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
	MojErr err = q.from(PopAccountAdapter::POP_ACCOUNT_KIND);
	ErrorToException(err);

	err = q.where(PopAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
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

void MojoDatabase::GetAccountFolders(Signal::SlotRef slot, const MojObject& accountId)
{
	MojDbQuery q;
	MojErr err = q.from(PopFolderAdapter::POP_FOLDER_KIND);
	ErrorToException(err);

	err = q.where(PopFolderAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetFolder(Signal::SlotRef slot, const MojObject& folderId)
{
	MojDbQuery q;
	MojErr err = q.from(PopFolderAdapter::POP_FOLDER_KIND);
	ErrorToException(err);

	err = q.where(PopFolderAdapter::ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}


void MojoDatabase::GetEmail(Signal::SlotRef slot, const MojObject& emailId)
{
	MojDbQuery q;

	MojErr err = q.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);

	err = q.where(PopEmailAdapter::ID, MojDbQuery::OpEq, emailId);
	ErrorToException(err);

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetEmailTransportObj(Signal::SlotRef slot, const MojObject& emailId)
{
	MojDbQuery q;

	MojErr err = q.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);

	err = q.where(PopEmailAdapter::ID, MojDbQuery::OpEq, emailId);
	ErrorToException(err);

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt32 limit)
{
	MojDbQuery q;
	MojErr err = q.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);

	err = q.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	err = q.order(EmailSchema::TIMESTAMP);
	ErrorToException(err);

	q.limit(limit);
	q.desc(true);

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetEmailSyncList(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, bool desc, MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(PopEmailAdapter::POP_EMAIL_KIND);//EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	// Select relevant fields
	err = query.select(PopEmailAdapter::ID);
	ErrorToException(err);

	err = query.select(PopEmailAdapter::SERVER_UID);
	ErrorToException(err);

	err = query.select(EmailSchema::TIMESTAMP);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	if (rev > 0) {
		err = query.where(PopFolderAdapter::LAST_SYNC_REV, MojDbQuery::OpGreaterThan, rev);
		ErrorToException(err);
	}

	query.page(page);

	// Sort by timestamp descending
	err = query.order(EmailSchema::TIMESTAMP);
	ErrorToException(err);
	query.desc(desc);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	slot.cancel(); // cancel existing slot in case we're in a callback

	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetLocalEmailChanges(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit)
{
	MojDbQuery query;

	MojErr err = query.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	if (rev > 0) {
		err = query.where(PopFolderAdapter::LAST_SYNC_REV, MojDbQuery::OpGreaterThan, rev);
		ErrorToException(err);
	}

	query.includeDeleted(true);
	query.page(page);

	slot.cancel();
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetAutoDownloadEmails(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit)
{
	MojDbQuery query;

	MojErr err = query.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);
	
	if (rev > 0) {
		err = query.where(PopFolderAdapter::LAST_SYNC_REV, MojDbQuery::OpGreaterThan, rev);
		ErrorToException(err);
	}

	query.page(page);

	// Sort by timestamp descending
	err = query.order(EmailSchema::TIMESTAMP);
	ErrorToException(err);
	query.desc(true);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	slot.cancel();
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetUidCache(Signal::SlotRef slot, const MojObject& accountId)
{
	MojDbQuery q;
	MojErr err = q.from(UidCacheAdapter::EMAILS_UID_CACHE_KIND);
	ErrorToException(err);

	err = q.where(UidCacheAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetSentEmails(Signal::SlotRef slot, const MojObject& outboxFolderId, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, outboxFolderId);
	ErrorToException(err);

	MojString sendStatus;
	err = sendStatus.format("%s.%s",EmailSchema::SEND_STATUS, EmailSchema::SendStatus::SENT);
	ErrorToException(err);
	err = query.where(sendStatus.data(), MojDbQuery::OpEq, true);
	ErrorToException(err);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	slot.cancel(); // cancel existing slot in case we're in a callback
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetDeletedEmails(Signal::SlotRef slot, const MojObject& folderId, const MojInt64& rev, MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);

	// Select relevant fields
	err = query.select(PopEmailAdapter::ID);
	ErrorToException(err);

	err = query.select(PopEmailAdapter::SERVER_UID);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	err = query.where("_del", MojDbQuery::OpEq, true);
	ErrorToException(err);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	// Make sure deleted objects are returned
	query.includeDeleted(true);

	// Set page
	query.page(page);

	slot.cancel(); // cancel existing slot in case we're in a callback
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetEmailsToMove(Signal::SlotRef slot, const MojObject& accountId)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);

	err = query.select(PopEmailAdapter::ID);
	ErrorToException(err);

	err = query.select("folderId");
	ErrorToException(err);

	err = query.select("destFolderId");
	ErrorToException(err);

	err = query.select("flags");
	ErrorToException(err);

	err = query.select(PopEmailAdapter::SERVER_UID);
	ErrorToException(err);

	err = query.where("destFolderId", MojDbQuery::OpNotEq, MojObject());
	ErrorToException(err);

	slot.cancel();
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetEmailsToDelete(Signal::SlotRef slot)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(PopEmailAdapter::POP_EMAIL_KIND);
	ErrorToException(err);

	err = query.select(PopEmailAdapter::ID);
	ErrorToException(err);

	err = query.select(PopEmailAdapter::SERVER_UID);
	ErrorToException(err);

	err = query.select(EmailSchema::FOLDER_ID);
	ErrorToException(err);

	err = query.where("_del", MojDbQuery::OpEq, true);
	ErrorToException(err);

	query.includeDeleted(true);

	slot.cancel();
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetById (Signal::SlotRef slot, const MojObject& id)
{
	MojErr err = m_dbClient.get(slot, id);
	ErrorToException(err);
}

void MojoDatabase::GetByIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids)
{
	MojErr err = m_dbClient.get(slot, ids.begin(), ids.end());
	ErrorToException(err);
}

void MojoDatabase::UpdateAccount(Signal::SlotRef slot, const MojObject& accountId, const MojObject& props)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(PopAccountAdapter::POP_ACCOUNT_KIND);
	ErrorToException(err);
	err = query.where(PopAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.merge(slot, query, props);
	ErrorToException(err);
}

void MojoDatabase::UpdateAccountRetry(Signal::SlotRef slot, const MojObject& accountId, const MojObject& account)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(PopAccountAdapter::POP_ACCOUNT_KIND);
	ErrorToException(err);
	err = query.where(PopAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.merge(slot, query, account);
	ErrorToException(err);
}

void MojoDatabase::UpdateAccountInitialSync(Signal::SlotRef slot, const MojObject& accountId, bool sync)
{
	//MojLogInfo(PopClient::s_log, "Updating account '%s' initialSynced to %d", AsJsonString(accountId).c_str(), sync);
	MojObject account;
	MojErr err = account.putBool(PopAccountAdapter::INITIAL_SYNC, sync);
	ErrorToException(err);

	MojDbQuery query;
	err = query.from(PopAccountAdapter::POP_ACCOUNT_KIND);
	ErrorToException(err);
	err = query.where(PopAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.merge(slot, query, account);
	ErrorToException(err);
}

void MojoDatabase::UpdateEmailParts(Signal::SlotRef slot, const MojObject& emailId, const MojObject& parts, bool autoDownload)
{
	MojObject email;
	MojErr err = email.put(PopEmailAdapter::ID, emailId);
	ErrorToException(err);

	err = email.put(EmailSchema::PARTS, parts);
	ErrorToException(err);

	if ((!parts.undefined() && !parts.null() && parts.size() > 0) || !autoDownload) {
		err = email.put(PopEmailAdapter::DOWNLOADED, true);
		ErrorToException(err);
	}

	MojLogDebug(PopClient::s_log, "Updating email '%s' with parts: '%s'", AsJsonString(emailId).c_str(), AsJsonString(parts).c_str());
	err = m_dbClient.merge(slot, email);
	ErrorToException(err);
}

void MojoDatabase::UpdateEmailSummary(Signal::SlotRef slot, const MojObject& emailId, const MojString& summary)
{
	MojObject email;
	MojErr err = email.put(PopEmailAdapter::ID, emailId);
	ErrorToException(err);

	err = email.put(EmailSchema::SUMMARY, summary);
	ErrorToException(err);

	//MojLogInfo(PopClient::s_log, "Updating email '%s' with summary: '%s'", AsJsonString(emailId).c_str(), AsJsonString(summary).c_str());
	err = m_dbClient.merge(slot, email);
	ErrorToException(err);
}

void MojoDatabase::UpdateAccountFolders(Signal::SlotRef slot, const MojObject& accountId, const MojObject& inboxFolderId,
																						  const MojObject& draftsFolderId,
																						  const MojObject& sentFolderId,
																						  const MojObject& outboxFolderId,
																						  const MojObject& trashFolderId)
{
	MojDbQuery q;
	MojErr err = q.from(PopAccountAdapter::POP_ACCOUNT_KIND);
	ErrorToException(err);

	err = q.where(PopAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	MojObject props;

	if (!inboxFolderId.undefined()) {
		err = props.put(EmailAccountAdapter::INBOX_FOLDERID, inboxFolderId);
		ErrorToException(err);
	}

	if (!draftsFolderId.undefined()) {
		err = props.put(EmailAccountAdapter::DRAFTS_FOLDERID, draftsFolderId);
		ErrorToException(err);
	}

	if (!sentFolderId.undefined()) {
		err = props.put(EmailAccountAdapter::SENT_FOLDERID, sentFolderId);
		ErrorToException(err);
	}

	if (!outboxFolderId.undefined()) {
		err = props.put(EmailAccountAdapter::OUTBOX_FOLDERID, outboxFolderId);
		ErrorToException(err);
	}

	if (!trashFolderId.undefined()) {
		err = props.put(EmailAccountAdapter::TRASH_FOLDERID, trashFolderId);
		ErrorToException(err);
	}

	err = m_dbClient.merge(slot, q, props);
	ErrorToException(err);
}

void MojoDatabase::MoveDeletedEmailToTrash (Signal::SlotRef slot, const MojObject& emailId, const MojObject& trashFolderId)
{
	MojObject email;
	MojErr err = email.put(PopEmailAdapter::ID, emailId);
	ErrorToException(err);

	err = email.put(EmailSchema::FOLDER_ID, trashFolderId);
	ErrorToException(err);

	err = email.put("_del", false);
	ErrorToException(err);

	MojLogInfo(PopClient::s_log, "Moving email '%s' to trash folder '%s'", AsJsonString(emailId).c_str(), AsJsonString(trashFolderId).c_str());
	err = m_dbClient.merge(slot, email);
	ErrorToException(err);
}

void MojoDatabase::GetExistingItems(Signal::SlotRef slot, const MojDbQuery& query)
{
	MojErr err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::ReserveIds(Signal::SlotRef slot, int count)
{
	if (count > RESERVE_ID_MAX)
		count = RESERVE_ID_MAX;
	MojErr err = m_dbClient.reserveIds(slot, count);
	ErrorToException(err);
}

void MojoDatabase::AddItems(Signal::SlotRef slot, const MojObject::ObjectVec& array)
{
	MojErr err = m_dbClient.put(slot, array.begin(), array.end());
	ErrorToException(err);
}

void MojoDatabase::UpdateItem(Signal::SlotRef slot, const MojObject& obj)
{
	MojErr err = m_dbClient.merge(slot, obj);
	ErrorToException(err);
}

void MojoDatabase::UpdateItems(Signal::SlotRef slot, const MojObject::ObjectVec& array)
{
	MojErr err = m_dbClient.merge(slot, array.begin(), array.end());
	ErrorToException(err);
}

void MojoDatabase::UpdateItems (Signal::SlotRef slot, const MojDbQuery query, const MojObject values)
{
	MojErr err = m_dbClient.merge(slot, query, values);
	ErrorToException(err);
}

void MojoDatabase::UpdateItemRevisions(Signal::SlotRef slot, const MojObject::ObjectVec& array)
{
	MojErr err = m_dbClient.merge(slot, array.begin(), array.end());
	ErrorToException(err);
}

void MojoDatabase::DeleteItems (Signal::SlotRef slot, const MojObject::ObjectVec& array)
{
	MojErr err = m_dbClient.del(slot, array.begin(), array.end(), MojDb::FlagPurge);
	ErrorToException(err);
}

void MojoDatabase::DeleteItems (Signal::SlotRef slot, const std::string kind, const std::string idField, const MojObject& id)
{
	MojDbQuery q;
	MojErr err = q.from(kind.c_str());
	ErrorToException(err);

	err = q.where(idField.c_str(), MojDbQuery::OpEq, id);
	ErrorToException(err);

	err = m_dbClient.del(slot, q, MojDb::FlagPurge);
	ErrorToException(err);
}
