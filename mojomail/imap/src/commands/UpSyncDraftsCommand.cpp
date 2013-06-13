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

#include "commands/UpSyncDraftsCommand.h"
#include "commands/ImapCommandResult.h"
#include "client/ImapSession.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "data/EmailSchema.h"
#include "data/EmailAdapter.h"
#include "data/ImapFolderAdapter.h"
#include "data/ImapEmail.h"
#include "data/ImapEmailAdapter.h"
#include "ImapPrivate.h"
#include "activity/ImapActivityFactory.h"
#include "activity/ActivityBuilder.h"
#include "activity/ActivitySet.h"
#include "commands/AppendCommand.h"
#include "commands/MoveEmailsCommand.h"


// FIXME low value for testing
const int UpSyncDraftsCommand::LOCAL_BATCH_SIZE = 10;

UpSyncDraftsCommand::UpSyncDraftsCommand(ImapSession& session, const MojObject& folderId)
: ImapSyncSessionCommand(session, folderId),
  m_oldUID(0),
  m_getDraftsFolderNameSlot(this, &UpSyncDraftsCommand::GetDraftsFolderNameResponse),
  m_getDraftsSlot(this, &UpSyncDraftsCommand::GetDraftsResponse),
  m_appendResponseSlot(this, &UpSyncDraftsCommand::AppendCommandResponse),
  m_updateUidAndKindSlot(this, &UpSyncDraftsCommand::UpdateUidAndKindResponse),
  m_deleteOldDraftSlot(this, &UpSyncDraftsCommand::DeleteOldDraftDone),
  m_replaceActivitySlot(this, &UpSyncDraftsCommand::ReplaceActivityDone)
{
}

UpSyncDraftsCommand::~UpSyncDraftsCommand()
{
}

void UpSyncDraftsCommand::RunImpl()
{
	GetDraftsFolderName();
}

void UpSyncDraftsCommand::GetDraftsFolderName()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().GetFolderName(m_getDraftsFolderNameSlot, m_folderId);
}

MojErr UpSyncDraftsCommand::GetDraftsFolderNameResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		MojObject folderObj;
		DatabaseAdapter::GetOneResult(response, folderObj);

		MojString draftsFolderName;
		err = folderObj.getRequired(ImapFolderAdapter::SERVER_FOLDER_NAME, draftsFolderName);
		ErrorToException(err);

		m_draftsFolderName.assign(draftsFolderName);

		GetDrafts();

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void UpSyncDraftsCommand::GetDrafts()
{
	CommandTraceFunction();

	MojObject draftsId = m_session.GetAccount()->GetDraftsFolderId();
	m_session.GetDatabaseInterface().GetDrafts(m_getDraftsSlot, draftsId, m_draftsPage, LOCAL_BATCH_SIZE);
}

MojErr UpSyncDraftsCommand::GetDraftsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		// Store all draft emails in a vector.
		BOOST_FOREACH(const MojObject& emailObj, DatabaseAdapter::GetResultsIterators(response))
		{
			EmailPtr email = boost::make_shared<ImapEmail>();

			EmailAdapter::ParseDatabaseObject(emailObj, *email);

			MojObject id;
			err = emailObj.getRequired(DatabaseAdapter::ID, id);
			ErrorToException(err);
			email->SetId(id);

			// Parse UID (only exists for IMAP emails already on server)
			bool hasUID = false;
			UID uid = 0;
			err = emailObj.get(ImapEmailAdapter::UID, uid, hasUID);
			ErrorToException(err);

			if(hasUID && uid > 0) {
				email->SetUID(uid);
			}

			m_drafts.push_back(email);
		}

		// Check for more results
		bool hasMoreResults = DatabaseAdapter::GetNextPage(response, m_draftsPage);

		if(hasMoreResults) {
			GetDrafts();
		} else {
			if(!m_drafts.empty()) {
				MojLogInfo(m_log, "uploading %d draft emails to the server", m_drafts.size());
				m_emailP = m_drafts.back();
				CreateAndRunAppendCommand();
			} else {
				SetupWatchDraftsActivity();
			}
		}

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void UpSyncDraftsCommand::CreateAndRunAppendCommand()
{
	CommandTraceFunction();

	m_appendCommand.reset((new AppendCommand(m_session, m_folderId, m_emailP, "\\Draft", m_draftsFolderName)));
	m_appendCommand->Run(m_appendResponseSlot);
}

MojErr UpSyncDraftsCommand::AppendCommandResponse()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "done appending draft");

	try
	{
		// If this fails, we should update the email anyways to avoid getting stuck
		// Possibly we could set a "numFailedAttempts" counter or something
		m_appendCommand->GetResult()->CheckException();

		UpdateUidAndKind();
	}
	CATCH_AS_FAILURE

	return MojErrNone;
}


void UpSyncDraftsCommand::UpdateUidAndKind()
{
	CommandTraceFunction();

	MojErr err;

	MojObject emailObj;

	err = emailObj.put(DatabaseAdapter::ID, m_emailP->GetId());
	ErrorToException(err);

	// Remove editedDraft flag so we don't try to upload it again
	MojObject modifiedFlags;
	err = modifiedFlags.put(EmailSchema::Flags::EDITEDDRAFT, false);
	ErrorToException(err);

	err = emailObj.put(EmailSchema::FLAGS, modifiedFlags);
	ErrorToException(err);

	// If UIDPLUS is supported, convert into an IMAP email
	if(m_session.GetCapabilities().HasCapability(Capabilities::UIDPLUS) && m_appendCommand->GetUid() > 0)
	{
		err = emailObj.putString(EmailSchema::KIND, ImapEmailAdapter::IMAP_EMAIL_KIND);
		ErrorToException(err);
		err = emailObj.putInt(ImapEmailAdapter::UID, m_appendCommand->GetUid());
		ErrorToException(err);

		// TODO set lastSyncFlags

		// FIXME check UID validity
		MojLogInfo(m_log, "updating draft email %s with UID=%u UIDVALIDITY=%u", AsJsonString(m_emailP->GetId()).c_str(), m_appendCommand->GetUid(), m_appendCommand->GetUidValidity());

		// Save old UID, if any
		m_oldUID = m_emailP->GetUID();
	} else {
		// FIXME: Delete from device and download new copy
	}

	m_session.GetDatabaseInterface().UpdateEmail(m_updateUidAndKindSlot, emailObj);
}

MojErr UpSyncDraftsCommand::UpdateUidAndKindResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		DeleteOldDraftFromServer();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

// Delete old draft from server
void UpSyncDraftsCommand::DeleteOldDraftFromServer()
{
	if(m_oldUID > 0) {
		m_deleteOldDraftSlot.cancel();

		std::vector<UID> uids;
		uids.push_back(m_oldUID);

		// make sure UID doesn't get deleted again by accident
		m_oldUID = 0;

		MojObject::ObjectVec ids; // empty

		MojLogInfo(m_log, "deleting existing draft email uid=%d from server", m_oldUID);

		m_moveCommand.reset(new MoveEmailsCommand(m_session, m_folderId, MojObject::Null, true, uids, ids));
		m_moveCommand->Run(m_deleteOldDraftSlot);
	} else {
		UpSyncDraftComplete();
	}
}

MojErr UpSyncDraftsCommand::DeleteOldDraftDone()
{
	try {
		UpSyncDraftComplete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void UpSyncDraftsCommand::UpSyncDraftComplete()
{
	CommandTraceFunction();

	m_drafts.pop_back();

	// Append the next draft email if there is more in the queue.
	if(!m_drafts.empty())
	{
		m_emailP = m_drafts.back();
		CreateAndRunAppendCommand();
	}
	else
	{
		SetupWatchDraftsActivity();
	}
}

void UpSyncDraftsCommand::SetupWatchDraftsActivity()
{
	CommandTraceFunction();

	MojObject accountId = m_session.GetAccount()->GetId();
	MojObject folderId = m_session.GetAccount()->GetDraftsFolderId();

	ImapActivityFactory factory;
	ActivityBuilder ab;

	factory.BuildDraftsWatch(ab, accountId, folderId, 0);

	m_commandActivitySet->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
	m_commandActivitySet->EndActivities(m_replaceActivitySlot);
}

MojErr UpSyncDraftsCommand::ReplaceActivityDone()
{
	CommandTraceFunction();

	Complete();

	return MojErrNone;
}

void UpSyncDraftsCommand::Status(MojObject& status) const
{
	MojErr err;
	ImapSyncSessionCommand::Status(status);

	if(m_appendCommand.get()) {
		MojObject appendCommandStatus;
		m_appendCommand->Status(appendCommandStatus);

		err = status.put("appendCommand", appendCommandStatus);
		ErrorToException(err);
	}

	if(m_moveCommand.get()) {
		MojObject moveCommandStatus;
		m_moveCommand->Status(moveCommandStatus);

		err = status.put("moveCommand", moveCommandStatus);
		ErrorToException(err);
	}
}
