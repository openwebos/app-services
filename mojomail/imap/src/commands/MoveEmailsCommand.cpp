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

#include "commands/MoveEmailsCommand.h"
#include "client/ImapSession.h"
#include "client/SyncSession.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailSchema.h"
#include "data/ImapEmailAdapter.h"
#include "data/ImapFolderAdapter.h"
#include "protocol/ImapResponseParser.h"
#include "ImapPrivate.h"
#include <sstream>

MoveEmailsCommand::MoveEmailsCommand(ImapSession& session, const MojObject& srcFolderId, const MojObject& destFolderId, bool deleteEmails, const std::vector<UID>& uids, const MojObject::ObjectVec& ids)
: ImapSyncSessionCommand(session, srcFolderId),
  m_destFolderId(destFolderId),
  m_deleteEmails(deleteEmails),
  m_uids(uids),
  m_ids(ids),
  m_getDestFolderSlot(this, &MoveEmailsCommand::GetDestFolderResponse),
  m_copyToDestSlot(this, &MoveEmailsCommand::CopyToDestResponse),
  m_deleteFromServerSlot(this, &MoveEmailsCommand::DeleteFromServerResponse),
  m_expungeSlot(this, &MoveEmailsCommand::ExpungeResponse),
  m_purgeSlot(this, &MoveEmailsCommand::PurgeEmailsResponse),
  m_unmoveEmailsSlot(this, &MoveEmailsCommand::UnmoveEmailsResponse)
{
}

MoveEmailsCommand::~MoveEmailsCommand()
{
}

void MoveEmailsCommand::RunImpl()
{
	if(IsValidId(m_destFolderId)) {
		GetDestFolder();
	} else {
		// Invalid or null destination
		if(m_deleteEmails) {
			DeleteFromServer();
		} else {
			// Probably shouldn't even get here
			UnmoveEmails();
		}
	}
}

void MoveEmailsCommand::GetDestFolder()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().GetFolderName(m_getDestFolderSlot, m_destFolderId);
}

MojErr MoveEmailsCommand::GetDestFolderResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		bool validFolder = false;
		MojObject folderObj;

		try {
			DatabaseAdapter::GetOneResult(response, folderObj);
			validFolder = true;
		} catch(const std::exception& e) {
			// Folder doesn't exist
			MojLogWarning(m_log, "attempted to copy emails to non-existent folder %s", AsJsonString(m_destFolderId).c_str());
			validFolder = false;
		}

		if(validFolder) {
			MojString folderName;
			err = folderObj.getRequired(ImapFolderAdapter::SERVER_FOLDER_NAME, folderName);
			ErrorToException(err);

			CopyToDest(folderName);
		} else {
			CopyFailed();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

// Copy emails to destination. Non-existant UIDs will be ignored.
void MoveEmailsCommand::CopyToDest(const MojString& destFolderName)
{
	CommandTraceFunction();

	assert(destFolderName.data());

	stringstream ss;

	ss << "UID COPY ";

	AppendUIDs(ss, m_uids.begin(), m_uids.end());

	ss << " " << QuoteString(destFolderName.data());

	m_copyResponseParser.reset(new ImapResponseParser(m_session, m_copyToDestSlot));

	m_session.SendRequest(ss.str(), m_copyResponseParser);
}

MojErr MoveEmailsCommand::CopyToDestResponse()
{
	CommandTraceFunction();

	try {
		// Check response status
		// In particular, this could fail if the trash folder no longer exists.
		if(m_copyResponseParser->GetStatus() != OK) {
			MojLogError(m_log, "error copying emails to destination folder: %s",
					m_copyResponseParser->GetResponseLine().c_str());
			CopyFailed();
		} else {
			DeleteFromServer();
		}

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void MoveEmailsCommand::CopyFailed()
{
	if(m_deleteEmails) {
		DeleteFromServer();
	} else {
		UnmoveEmails();
	}
}

void MoveEmailsCommand::DeleteFromServer()
{
	CommandTraceFunction();

	stringstream ss;
	ss << "UID STORE ";

	AppendUIDs(ss, m_uids.begin(), m_uids.end());

	ss << " +FLAGS.SILENT (\\Deleted)";

	m_deleteResponseParser.reset(new ImapResponseParser(m_session, m_deleteFromServerSlot));

	m_session.SendRequest(ss.str(), m_deleteResponseParser);
}

MojErr MoveEmailsCommand::DeleteFromServerResponse()
{
	CommandTraceFunction();

	try {
		m_deleteResponseParser->CheckStatus();

		if(m_session.GetAccount()->GetEnableExpunge())
			Expunge();
		else
			PurgeEmails();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void MoveEmailsCommand::Expunge()
{
	CommandTraceFunction();

	m_expungeResponseParser.reset(new ImapResponseParser(m_session, m_expungeSlot));

	stringstream ss;

	// If supported, use the UID EXPUNGE command which doesn't interfere with other clients as much
	if(m_session.GetCapabilities().HasCapability(Capabilities::UIDPLUS)) {
		ss << "UID EXPUNGE ";
		AppendUIDs(ss, m_uids.begin(), m_uids.end());
	} else {
		ss << "EXPUNGE";
	}

	m_session.SendRequest(ss.str(), m_expungeResponseParser);
}

MojErr MoveEmailsCommand::ExpungeResponse()
{
	CommandTraceFunction();

	try {
		if(m_expungeResponseParser->GetStatus() == NO) {
			MojLogError(m_log, "error expunging: %s", m_expungeResponseParser->GetResponseLine().c_str());
			// ignore error
		} else {
			m_expungeResponseParser->CheckStatus();
		}

		PurgeEmails();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void MoveEmailsCommand::PurgeEmails()
{
	// Delete email from device
	m_session.GetDatabaseInterface().DeleteEmailIds(m_purgeSlot, m_ids);
}

MojErr MoveEmailsCommand::PurgeEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		m_syncSession->AddPutResponseRevs(response);

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

// Remove destFolderId from selected emails
void MoveEmailsCommand::UnmoveEmails()
{
	CommandTraceFunction();

	MojErr err;
	MojObject::ObjectVec toMerge;

	BOOST_FOREACH(const MojObject& id, make_pair(m_ids.begin(), m_ids.end())) {
		MojObject obj;

		err = obj.put(DatabaseAdapter::ID, id);
		ErrorToException(err);

		err = obj.put(ImapEmailAdapter::DEST_FOLDER_ID, MojObject::Null);
		ErrorToException(err);

		MojObject flags;

		err = flags.put(EmailSchema::Flags::VISIBLE, true);
		ErrorToException(err);

		err = obj.put(EmailSchema::FLAGS, flags);
		ErrorToException(err);

		err = toMerge.push(obj);
		ErrorToException(err);
	}

	// FIXME should create a new method not called MergeFlags
	m_session.GetDatabaseInterface().MergeFlags(m_unmoveEmailsSlot, toMerge);
}

MojErr MoveEmailsCommand::UnmoveEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		m_syncSession->AddPutResponseRevs(response);

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}
