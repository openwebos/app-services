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

#include "commands/HandleUpdateCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "data/EmailSchema.h"
#include "data/ImapEmailAdapter.h"
#include "client/ImapSession.h"
#include "client/SyncSession.h"

HandleUpdateCommand::HandleUpdateCommand(ImapSession& session, const MojObject& folderId, UID uid, bool deleted, const MojObject& newFlags)
: ImapSyncSessionCommand(session, folderId),
  m_uid(uid),
  m_deleted(deleted),
  m_newFlags(newFlags),
  m_getEmailSlot(this, &HandleUpdateCommand::GetEmailResponse),
  m_deleteSlot(this, &HandleUpdateCommand::DeleteResponse),
  m_updateFlagsSlot(this, &HandleUpdateCommand::UpdateFlagsResponse)
{
}

HandleUpdateCommand::~HandleUpdateCommand()
{
}

void HandleUpdateCommand::RunImpl()
{
	CommandTraceFunction();

	GetEmail();
}

void HandleUpdateCommand::GetEmail()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().GetEmail(m_getEmailSlot, m_folderId, m_uid);
}

MojErr HandleUpdateCommand::GetEmailResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		MojObject results;
		err = response.getRequired(DatabaseAdapter::RESULTS, results);
		ErrorToException(err);

		if(results.size() == 1) {
			MojObject email;
			if(!results.at(0, email)) {
				throw MailException("Error getting email", __FILE__, __LINE__);
			}

			err = email.getRequired(DatabaseAdapter::ID, m_emailId);
			ErrorToException(err);

			if(m_deleted) {
				DeleteEmail();
			} else {
				UpdateFlags();
			}
		} else {
			// If an email with this UID doesn't exist, that's OK.
			// We may have already deleted it, or never downloaded it in the first place.
			Done();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void HandleUpdateCommand::DeleteEmail()
{
	CommandTraceFunction();

	MojErr err;
	MojObject::ObjectVec ids;

	err = ids.push(m_emailId);
	ErrorToException(err);

	m_session.GetDatabaseInterface().DeleteEmailIds(m_deleteSlot, ids);
}

MojErr HandleUpdateCommand::DeleteResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		m_syncSession->AddPutResponseRevs(response);

		Done();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void HandleUpdateCommand::UpdateFlags()
{
	CommandTraceFunction();

	MojErr err;

	MojObject::ObjectVec objects;

	// TODO don't overwrite user changes

	MojObject toMerge;
	err = toMerge.put(DatabaseAdapter::ID, m_emailId);
	ErrorToException(err);
	err = toMerge.put(EmailSchema::FLAGS, m_newFlags);
	ErrorToException(err);
	err = toMerge.put(ImapEmailAdapter::LAST_SYNC_FLAGS, m_newFlags);
	ErrorToException(err);

	err = objects.push(toMerge);
	ErrorToException(err);

	m_session.GetDatabaseInterface().MergeFlags(m_updateFlagsSlot, objects);
}

MojErr HandleUpdateCommand::UpdateFlagsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		Done();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void HandleUpdateCommand::Done()
{
	CommandTraceFunction();

	Complete();
}
