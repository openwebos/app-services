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

#include "data/MojoDatabase.h"
#include "ImapCoreDefs.h"
#include "db/MojDbQuery.h"
#include "data/EmailAccountAdapter.h"
#include "data/EmailSchema.h"
#include "data/Folder.h"
#include "data/FolderAdapter.h"
#include "data/ImapEmailAdapter.h"
#include "data/ImapFolderAdapter.h"
#include "data/DatabaseAdapter.h"

MojoDatabase::MojoDatabase(MojDbClient& dbClient)
: m_dbClient(dbClient)
{
}

MojoDatabase::~MojoDatabase()
{
}

void MojoDatabase::Find(Signal::SlotRef slot, MojDbQuery query)
{
	MojErr err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetById(Signal::SlotRef slot, const MojObject& id)
{
	MojErr err = m_dbClient.get(slot, id);
	ErrorToException(err);
}

void MojoDatabase::GetByIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids)
{
	MojErr err = m_dbClient.get(slot, ids.begin(), ids.end());
	ErrorToException(err);
}

void MojoDatabase::GetEmail(Signal::SlotRef slot, const MojObject& folderId, UID uid)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapEmailAdapter::IMAP_EMAIL_KIND);
	ErrorToException(err);
	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);
	err = query.where(ImapEmailAdapter::UID, MojDbQuery::OpEq, (MojInt64) uid);
	ErrorToException(err);

	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::CreateFolders(Signal::SlotRef slot, const MojObject::ObjectVec& array)
{
	MojErr err = m_dbClient.put(slot, array.begin(), array.end());
	ErrorToException(err);
}

void MojoDatabase::ReserveIds(Signal::SlotRef slot, MojUInt32 num)
{
	MojErr err = m_dbClient.reserveIds(slot, num);
	ErrorToException(err);
}

void MojoDatabase::DeleteIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids)
{
	MojErr err = m_dbClient.del(slot, ids.begin(), ids.end());
	ErrorToException(err);
}

void MojoDatabase::PurgeIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids)
{
	MojErr err = m_dbClient.del(slot, ids.begin(), ids.end(), MojDb::FlagPurge);
	ErrorToException(err);
}

void MojoDatabase::DeleteFolderEmails(Signal::SlotRef slot, const MojObject& folderId)
{
	MojErr err;
	MojDbQuery query;
	
	err = query.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	// Delete and purge all emails in this folder
	err = m_dbClient.del(slot, query, MojDb::FlagPurge);
	ErrorToException(err);
}

void MojoDatabase::DeleteAccount(Signal::SlotRef slot, const MojObject& accountId)
{
	MojDbQuery q;

	MojErr err = q.from(ImapAccountAdapter::SCHEMA);
	ErrorToException(err);

	err = q.where(ImapAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	// now delete the account
	err = m_dbClient.del(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetAccount(Signal::SlotRef slot, const MojObject& accountId)
{
	MojDbQuery q;

	MojErr err = q.from(ImapAccountAdapter::SCHEMA);
	ErrorToException(err);

	err = q.where(ImapAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	// now find the folder the e-mail belongs to
	err = m_dbClient.find(slot, q);
	ErrorToException(err);
}

void MojoDatabase::GetAutoDownloads(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapEmailAdapter::IMAP_EMAIL_KIND);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	// Select relevant fields
	err = query.select(DatabaseAdapter::ID);
	ErrorToException(err);

	err = query.select(EmailSchema::PARTS);
	ErrorToException(err);

	err = query.select(ImapEmailAdapter::AUTO_DOWNLOAD);
	ErrorToException(err);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	// Sort by timestamp descending
	err = query.order(EmailSchema::TIMESTAMP);
	ErrorToException(err);

	query.desc(true);

	// Page key
	query.page(page);

	// Cancel existing slot
	slot.cancel();

	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetDeletedEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapEmailAdapter::IMAP_EMAIL_KIND);
	ErrorToException(err);
	
	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	err = query.where("_del", MojDbQuery::OpEq, true); // FIXME use string constant
	ErrorToException(err);

	if(rev > 0) {
		err = query.where(ImapEmailAdapter::UPSYNC_REV, MojDbQuery::OpGreaterThan, rev);
		ErrorToException(err);
	}

	// FIXME: MojoDB doesn't seem to include the _del property if you try to select individual fields
	/*
	// Select relevant fields
	err = query.select(DatabaseAdapter::ID);
	ErrorToException(err);

	err = query.select("_del"); // FIXME use string constant
	ErrorToException(err);
	
	err = query.select(ImapEmailAdapter::UID);
	ErrorToException(err);

	err = query.select(DatabaseAdapter::REV);
	ErrorToException(err);
	*/

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	// Set page
	query.page(page);

	slot.cancel(); // cancel existing slot in case we're in a callback
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetMovedEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapEmailAdapter::IMAP_EMAIL_KIND);
	ErrorToException(err);
	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	if(rev > 0) {
		err = query.where(ImapEmailAdapter::UPSYNC_REV, MojDbQuery::OpGreaterThan, rev);
		ErrorToException(err);
	}

	// Select relevant fields
	err = query.select(DatabaseAdapter::ID);
	ErrorToException(err);
	err = query.select(DatabaseAdapter::REV);
	ErrorToException(err);
	err = query.select(ImapEmailAdapter::UID);
	ErrorToException(err);
	err = query.select(ImapEmailAdapter::DEST_FOLDER_ID);
	ErrorToException(err);

	query.page(page);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	slot.cancel(); // cancel existing slot in case we're in a callback
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetSentEmails(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	MojString sendStatus;
	sendStatus.format("%s.%s",EmailSchema::SEND_STATUS, EmailSchema::SendStatus::SENT);
	err = query.where(sendStatus.data(), MojDbQuery::OpEq, true);
	ErrorToException(err);

	query.page(page);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	slot.cancel(); // cancel existing slot in case we're in a callback
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetDrafts(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(EmailSchema::Kind::EMAIL);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	MojString editedDraftFlag;
	editedDraftFlag.format("%s.%s", EmailSchema::FLAGS, EmailSchema::Flags::EDITEDDRAFT);
	err = query.where(editedDraftFlag.data(), MojDbQuery::OpEq, true);
	ErrorToException(err);

	query.page(page);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	slot.cancel(); // cancel existing slot in case we're in a callback
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetEmailChanges(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapEmailAdapter::IMAP_EMAIL_KIND);
	ErrorToException(err);
	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	if(rev > 0) {
		err = query.where(ImapEmailAdapter::UPSYNC_REV, MojDbQuery::OpGreaterThan, rev);
		ErrorToException(err);
	}

	// Select relevant fields
	err = query.select(DatabaseAdapter::ID);
	ErrorToException(err);
	err = query.select(DatabaseAdapter::REV);
	ErrorToException(err);
	err = query.select(ImapEmailAdapter::UID);
	ErrorToException(err);
	err = query.select(EmailSchema::FLAGS);
	ErrorToException(err);
	err = query.select(ImapEmailAdapter::LAST_SYNC_FLAGS);
	ErrorToException(err);

	// Include deleted emails
	err = query.includeDeleted();
	ErrorToException(err);

	query.page(page);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	slot.cancel(); // cancel existing slot in case we're in a callback
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetEmailSyncList(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapEmailAdapter::IMAP_EMAIL_KIND);
	ErrorToException(err);

	err = query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	// Select relevant fields
	err = query.select("_id"); // FIXME use string constant
	ErrorToException(err);

	err = query.select(ImapEmailAdapter::UID);
	ErrorToException(err);

	err = query.select(EmailSchema::FLAGS);
	ErrorToException(err);

	err = query.select(ImapEmailAdapter::LAST_SYNC_FLAGS);
	ErrorToException(err);

	query.page(page);

	// Sort by UID ascending
	err = query.order(ImapEmailAdapter::UID);
	ErrorToException(err);

	// Set limit
	if(limit > 0) {
		query.limit(limit);
	}

	slot.cancel(); // cancel existing slot in case we're in a callback
	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetFolders(Signal::SlotRef slot, const MojObject& accountId, const MojDbQuery::Page& page, bool allFolders)
{
	MojErr err;
	MojDbQuery query;

	err = query.from(allFolders ? FolderAdapter::FOLDER_KIND : ImapFolderAdapter::IMAP_FOLDER_KIND);
	ErrorToException(err);

	err = query.where(FolderAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	query.page(page);

	slot.cancel(); // cancel existing slot in case we're in a callback

	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::GetFolderName(Signal::SlotRef slot, const MojObject& folderId)
{
	MojErr err;
	MojDbQuery query;

	err = query.from(ImapFolderAdapter::IMAP_FOLDER_KIND);
	ErrorToException(err);
	err = query.where(FolderAdapter::ID, MojDbQuery::OpEq, folderId);
	ErrorToException(err);

	// Select relevant fields
	err = query.select(ImapFolderAdapter::SERVER_FOLDER_NAME);
	ErrorToException(err);

	err = m_dbClient.find(slot, query);
	ErrorToException(err);
}

void MojoDatabase::UpdateAccount(Signal::SlotRef slot, const MojObject& accountId, const MojObject& props)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapAccountAdapter::SCHEMA);
	ErrorToException(err);
	err = query.where(ImapAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.merge(slot, query, props);
	ErrorToException(err);
}

void MojoDatabase::UpdateAccountSpecialFolders(Signal::SlotRef slot, const ImapAccount& imapAccount)
{
	MojErr err;
	MojObject props;

	MojDbQuery query;
	err = query.from(ImapAccountAdapter::SCHEMA);
	ErrorToException(err);
	err = query.where(ImapAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, imapAccount.GetId());
	ErrorToException(err);

	EmailAccountAdapter::SerializeSpecialFolders(imapAccount, props);

	err = m_dbClient.merge(slot, query, props);
	ErrorToException(err);
}

void MojoDatabase::UpdateAccountError(Signal::SlotRef slot, const MojObject& accountId, const MojObject& account)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapAccountAdapter::SCHEMA);
	ErrorToException(err);
	err = query.where(ImapAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.merge(slot, query, account);
	ErrorToException(err);
}

void MojoDatabase::UpdateAccountRetry(Signal::SlotRef slot, const MojObject& accountId, const MojObject& account)
{
	MojErr err;

	MojDbQuery query;
	err = query.from(ImapAccountAdapter::SCHEMA);
	ErrorToException(err);
	err = query.where(ImapAccountAdapter::ACCOUNT_ID, MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	err = m_dbClient.merge(slot, query, account);
	ErrorToException(err);
}

void MojoDatabase::PutEmails(Signal::SlotRef slot, const MojObject::ObjectVec& array)
{
	MojErr err = m_dbClient.put(slot, array.begin(), array.end());
	ErrorToException(err);
}

void MojoDatabase::MergeFlags(Signal::SlotRef slot, const MojObject::ObjectVec& array)
{
	MojErr err = m_dbClient.merge(slot, array.begin(), array.end());
	ErrorToException(err);
}

void MojoDatabase::UpdateEmails(Signal::SlotRef slot, const MojObject::ObjectVec& objects)
{
	MojErr err = m_dbClient.merge(slot, objects.begin(), objects.end());
	ErrorToException(err);
}

void MojoDatabase::DeleteEmailIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids)
{
	MojErr err;
	MojObject::ObjectVec toMerge;

	for(MojObject::ObjectVec::ConstIterator it = ids.begin(); it != ids.end(); ++it) {
		MojObject obj;

		err = obj.put(DatabaseAdapter::ID, *it);
		ErrorToException(err);

		err = obj.put("_del", true); // FIXME use constant
		ErrorToException(err);

		err = obj.put(ImapEmailAdapter::UID, MojObject::Null);
		ErrorToException(err);

		err = toMerge.push(obj);
		ErrorToException(err);
	}

	err = m_dbClient.merge(slot, toMerge.begin(), toMerge.end());
	ErrorToException(err);
}

void MojoDatabase::UpdateEmail(Signal::SlotRef slot, const MojObject& obj)
{
	MojErr err = m_dbClient.merge(slot, obj);
	ErrorToException(err);
}

void MojoDatabase::UpdateFolder(Signal::SlotRef slot, const MojObject& obj)
{
	MojErr err = m_dbClient.merge(slot, obj);
	ErrorToException(err);
}
