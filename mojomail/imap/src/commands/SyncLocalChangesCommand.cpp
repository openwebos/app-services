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

#include "commands/SyncLocalChangesCommand.h"
#include "client/ImapSession.h"
#include "client/SyncSession.h"
#include "commands/MoveEmailsCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "data/EmailSchema.h"
#include "data/ImapEmail.h"
#include "data/ImapEmailAdapter.h"
#include "data/ImapFolderAdapter.h"
#include "protocol/StoreResponseParser.h"
#include "ImapPrivate.h"
#include <sstream>

SyncLocalChangesCommand::SyncLocalChangesCommand(ImapSession& session, const MojObject& folderId)
: ImapSyncSessionCommand(session, folderId),
  m_expunge(true),
  m_highestRevSeen(0),
  m_getChangedEmailsSlot(this, &SyncLocalChangesCommand::GetChangedEmailsResponse),
  m_mergeFlagsSlot(this, &SyncLocalChangesCommand::MergeFlagsResponse),
  m_getDeletedEmailsSlot(this, &SyncLocalChangesCommand::GetDeletedEmailsResponse),
  m_getMovedEmailsSlot(this, &SyncLocalChangesCommand::GetMovedEmailsResponse),
  m_storeResponseSlot(this, &SyncLocalChangesCommand::StoreResponse),
  m_moveEmailsSlot(this, &SyncLocalChangesCommand::MoveEmailsToFolderDone),
  m_deleteEmailsSlot(this, &SyncLocalChangesCommand::DeleteEmailsDone)
{
}

SyncLocalChangesCommand::~SyncLocalChangesCommand()
{
}

void SyncLocalChangesCommand::RunImpl()
{
	// Flags get sync'd first, so that read+deleted emails are marked read before moving to trash
	GetChangedEmails();
}

void SyncLocalChangesCommand::GetChangedEmails()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().GetEmailChanges(m_getChangedEmailsSlot, m_folderId, GetLastSyncRev(), m_localChangesPage);
}

MojErr SyncLocalChangesCommand::GetChangedEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		// Read email properties: _id, UID, flags, lastSyncFlags
		// If you need additional properties, modify the database query
		BOOST_FOREACH(const MojObject& emailObj, DatabaseAdapter::GetResultsIterators(response)) {
			MojObject flagsObj, lastSyncFlagsObj;
			EmailFlags flags, lastSyncFlags;

			MojObject id;
			err = emailObj.getRequired(DatabaseAdapter::ID, id);
			ErrorToException(err);

			MojInt64 rev;
			err = emailObj.getRequired(DatabaseAdapter::REV, rev);
			ErrorToException(err);

			if(rev > m_highestRevSeen) {
				m_highestRevSeen = rev;
			}

			MojObject uidObj;
			if(emailObj.get(ImapEmailAdapter::UID, uidObj) && uidObj.null()) {
				// Skip deleted emails that have already been processed
				continue;
			}

			UID uid;
			err = emailObj.getRequired(ImapEmailAdapter::UID, uid);
			ErrorToException(err);

			if (uid == 0) {
				// Uh oh ... something bad going on
				MojLogWarning(m_log, "email %s had invalid UID 0; skipping", AsJsonString(id).c_str());
				continue;
			}

			if(emailObj.contains(EmailSchema::FLAGS)) {
				err = emailObj.getRequired(EmailSchema::FLAGS, flagsObj);
				ErrorToException(err);
				ImapEmailAdapter::ParseEmailFlags(flagsObj, flags);
			}

			if(emailObj.contains(ImapEmailAdapter::LAST_SYNC_FLAGS)) {
				err = emailObj.getRequired(ImapEmailAdapter::LAST_SYNC_FLAGS, lastSyncFlagsObj);
				ErrorToException(err);
				ImapEmailAdapter::ParseEmailFlags(lastSyncFlagsObj, lastSyncFlags);
			}

			bool modified = false;
			if(flags.read != lastSyncFlags.read) {
				if(flags.read) {
					m_plusSeen.push_back(uid);
				} else {
					m_minusSeen.push_back(uid);
				}
				modified = true;
			}

			if(flags.replied != lastSyncFlags.replied) {
				if(flags.replied) {
					m_plusAnswered.push_back(uid);
				} else {
					m_minusAnswered.push_back(uid);
				}
				modified = true;
			}

			if(flags.flagged != lastSyncFlags.flagged) {
				if(flags.flagged) {
					m_plusFlagged.push_back(uid);
				} else {
					m_minusFlagged.push_back(uid);
				}
				modified = true;
			}

			if(modified) {
				MojObject mergeObj;

				// Copy id
				mergeObj.put(DatabaseAdapter::ID, id);

				// Copy flags to lastSyncFlags
				err = mergeObj.put(ImapEmailAdapter::LAST_SYNC_FLAGS, flagsObj);
				ErrorToException(err);

				m_pendingMerge.push(mergeObj);
			}
		}

		// Check for more results
		bool hasMoreResults = DatabaseAdapter::GetNextPage(response, m_localChangesPage);

		if(hasMoreResults) {
			GetChangedEmails();
		} else {
			SendRequests();
		}

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncLocalChangesCommand::SendRequests()
{
	m_storeResponseParser.reset(new StoreResponseParser(m_session, m_storeResponseSlot));

	int requests = 0;

	// Seen
	if(!m_plusSeen.empty()) {
		requests++;
		SendStoreRequest(m_storeResponseParser, "+FLAGS.SILENT (\\Seen)", m_plusSeen);
	}

	if(!m_minusSeen.empty()) {
		requests++;
		SendStoreRequest(m_storeResponseParser, "-FLAGS.SILENT (\\Seen)", m_minusSeen);
	}

	// Answered
	if(!m_plusAnswered.empty()) {
		requests++;
		SendStoreRequest(m_storeResponseParser, "+FLAGS.SILENT (\\Answered)", m_plusAnswered);
	}

	if(!m_minusAnswered.empty()) {
		requests++;
		SendStoreRequest(m_storeResponseParser, "-FLAGS.SILENT (\\Answered)", m_minusAnswered);
	}

	// Flagged
	if(!m_plusFlagged.empty()) {
		requests++;
		SendStoreRequest(m_storeResponseParser, "+FLAGS.SILENT (\\Flagged)", m_plusFlagged);
	}

	if(!m_minusFlagged.empty()) {
		requests++;
		SendStoreRequest(m_storeResponseParser, "-FLAGS.SILENT (\\Flagged)", m_minusFlagged);
	}

	if(requests) {
		MojLogInfo(m_log, "sent %d flag update commands for %d emails", requests, m_pendingMerge.size());
	} else {
		MojLogInfo(m_log, "no local flag changes");

		FlagSyncComplete();
	}
}

/**
 * Send a flag update request. It's OK if some emails are non-existent on the server, per RFC 3501 section 6.4.8:
 * "A non-existent unique identifier is ignored without any error message generated."
 */
void SyncLocalChangesCommand::SendStoreRequest(MojRefCountedPtr<StoreResponseParser>& parser, const string& items, const vector<UID>& uids)
{
	stringstream ss;
	ss << "UID STORE ";

	AppendUIDs(ss, uids.begin(), uids.end());
	ss << " " << items;

	parser->AddExpectedResponse();
	m_session.SendRequest(ss.str(), parser);
}

// This will get called after *all* STORE commands complete (successfully or unsuccessfully)
MojErr SyncLocalChangesCommand::StoreResponse()
{
	CommandTraceFunction();

	// FIXME check status
	try {
		m_storeResponseParser->CheckStatus();

		MergeFlags();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncLocalChangesCommand::MergeFlags()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().MergeFlags(m_mergeFlagsSlot, m_pendingMerge);
}

MojErr SyncLocalChangesCommand::MergeFlagsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		m_pendingMerge.clear();

		FlagSyncComplete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncLocalChangesCommand::FlagSyncComplete()
{
	GetMovedEmails();
}

void SyncLocalChangesCommand::GetMovedEmails()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().GetMovedEmails(m_getMovedEmailsSlot, m_folderId, GetLastSyncRev(), m_movedEmailsPage);
}

MojErr SyncLocalChangesCommand::GetMovedEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		// Read email properties: _id, UID, destFolderId

		BOOST_FOREACH(const MojObject& emailObj, DatabaseAdapter::GetResultsIterators(response)) {
			MojObject destFolderId, id;
			UID uid;

			err = emailObj.getRequired(DatabaseAdapter::ID, id);
			ErrorToException(err);

			err = emailObj.getRequired(ImapEmailAdapter::UID, uid);
			ErrorToException(err);

			MojInt64 rev;
			err = emailObj.getRequired(DatabaseAdapter::REV, rev);
			ErrorToException(err);

			if(rev > m_highestRevSeen) {
				m_highestRevSeen = rev;
			}

			bool moved = false;
			if(emailObj.contains(ImapEmailAdapter::DEST_FOLDER_ID)) {
				err = emailObj.getRequired(ImapEmailAdapter::DEST_FOLDER_ID, destFolderId);
				ErrorToException(err);
				// Check if the folder Id is valid.
				if(IsValidId(destFolderId) && destFolderId != m_folderId)
					moved = true;
			}


			if(moved) {
				// There could be multiple destination folders.
				// We need to keep a set of distinct folder IDs and
				// handle one folder at a time.  We remove the first
				// folder ID from the set at the end of a move and do
				// further moving if the set is not empty.
				m_folderIdSet.insert(destFolderId);

				// We use multimaps to keep the 1:* mapping between
				// destination folder Id to mail Ids.
				m_folderIdUidMap.insert(pair<MojObject,UID>(destFolderId, uid));
				m_folderIdIdMap.insert(pair<MojObject,MojObject>(destFolderId, id));
			}
		}

		// Check for more results
		bool hasMoreResults = DatabaseAdapter::GetNextPage(response, m_movedEmailsPage);

		if(hasMoreResults) {
			GetMovedEmails();
		} else {
			if(!m_folderIdUidMap.empty()) {
				MojLogInfo(m_log, "moving %d emails on server", m_folderIdUidMap.size());
				MoveEmails();
			} else {
				MoveComplete();
			}
		}

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncLocalChangesCommand::MoveEmails()
{
	if(!m_folderIdSet.empty()) {
		MojObject destFolderId = *m_folderIdSet.begin();

		vector<UID> uids;
		typedef pair<MojObject, UID> UIDPair;
		BOOST_FOREACH(const UIDPair& p, m_folderIdUidMap.equal_range(destFolderId)) {
			uids.push_back(p.second);
		}

		MojObject::ObjectVec ids;
		typedef pair<MojObject, MojObject> IDPair;
		BOOST_FOREACH(const IDPair& p, m_folderIdIdMap.equal_range(destFolderId)) {
			MojErr err = ids.push(p.second);
			ErrorToException(err);
		}

		m_moveEmailsCommand.reset(new MoveEmailsCommand(m_session, m_folderId, destFolderId, false, uids, ids));
		m_moveEmailsCommand->Run(m_moveEmailsSlot);
	} else {
		MoveComplete();
	}
}

MojErr SyncLocalChangesCommand::MoveEmailsToFolderDone()
{
	try {
		m_moveEmailsCommand->GetResult()->CheckException();

		// pop folderId from set
		m_folderIdSet.erase( m_folderIdSet.begin() );

		MoveEmails();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncLocalChangesCommand::MoveComplete()
{
	GetDeletedEmails();
}

void SyncLocalChangesCommand::GetDeletedEmails()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().GetDeletedEmails(m_getDeletedEmailsSlot, m_folderId, GetLastSyncRev(), m_deletedEmailsPage);
}

MojErr SyncLocalChangesCommand::GetDeletedEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		BOOST_FOREACH(const MojObject& emailObj, DatabaseAdapter::GetResultsIterators(response)) {
			// sanity check: make sure _del is true
			bool deleted = false;
			err = emailObj.getRequired(DatabaseAdapter::DEL, deleted);
			ErrorToException(err);

			MojObject uidObj;
			if(emailObj.get(ImapEmailAdapter::UID, uidObj)) {
				if(uidObj.null()) {
					deleted = false;
				}
			} else {
				// UID doesn't exist
				deleted = false;
			}

			if(deleted) {
				MojObject id;
				err = emailObj.getRequired(DatabaseAdapter::ID, id);
				ErrorToException(err);

				UID uid;
				err = emailObj.getRequired(ImapEmailAdapter::UID, uid);
				ErrorToException(err);

				MojInt64 rev;
				err = emailObj.getRequired(DatabaseAdapter::REV, rev);
				ErrorToException(err);

				if(rev > m_highestRevSeen) {
					m_highestRevSeen = rev;
				}

				m_uidsToDelete.push_back(uid);
				m_pendingDelete.push(id);
			}
		}

		if(DatabaseAdapter::GetNextPage(response, m_deletedEmailsPage)) {
			GetDeletedEmails();
		} else {
			if(!m_uidsToDelete.empty()) {
				MojLogInfo(m_log, "deleting %d emails from server", m_uidsToDelete.size());
				DeleteEmails();
			} else {
				DeleteComplete();
			}
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncLocalChangesCommand::DeleteEmails()
{
	MojObject trashFolderId = m_session.GetAccount()->GetTrashFolderId();

	// If we're in the trash folder, just delete the message
	if(m_folderId == trashFolderId) {
		trashFolderId = MojObject::Null;
	}

	m_moveDeleteEmailsCommand.reset(new MoveEmailsCommand(m_session, m_folderId, trashFolderId, true, m_uidsToDelete, m_pendingDelete));
	m_moveDeleteEmailsCommand->Run(m_deleteEmailsSlot);
}

MojErr SyncLocalChangesCommand::DeleteEmailsDone()
{
	try {
		m_moveDeleteEmailsCommand->GetResult()->CheckException();

		DeleteComplete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncLocalChangesCommand::DeleteComplete()
{
	m_syncSession->SetNextSyncRev(m_highestRevSeen);
	Complete();
}

void SyncLocalChangesCommand::Cleanup()
{
	// Must clean this up since the signal doesn't fire if there's no changes
	m_storeResponseSlot.cancel();

	// Clean up just to be safe
	m_getChangedEmailsSlot.cancel();
	m_mergeFlagsSlot.cancel();
	m_getDeletedEmailsSlot.cancel();
	m_getMovedEmailsSlot.cancel();
	m_storeResponseSlot.cancel();
	m_moveEmailsSlot.cancel();
	m_deleteEmailsSlot.cancel();

	m_moveEmailsCommand.reset();
	m_moveDeleteEmailsCommand.reset();
}

void SyncLocalChangesCommand::Status(MojObject& status) const
{
	MojErr err;

	ImapSyncSessionCommand::Status(status);

	if(m_moveEmailsCommand.get()) {
		MojObject commandStatus;
		m_moveEmailsCommand->Status(commandStatus);

		err = status.put("moveEmailsCommand", commandStatus);
		ErrorToException(err);
	}

	if(m_moveDeleteEmailsCommand.get()) {
		MojObject commandStatus;
		m_moveDeleteEmailsCommand->Status(commandStatus);

		err = status.put("moveDeleteEmailsCommand", commandStatus);
		ErrorToException(err);
	}

}
