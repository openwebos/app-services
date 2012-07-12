// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "commands/DeleteServerEmailsCommand.h"
#include "commands/PopCommandResult.h"

DeleteServerEmailsCommand::DeleteServerEmailsCommand(PopSession& session,
		const ReconcileEmailsCommand::LocalDeletedEmailsVec& localDeletedEmailUids,
		UidCache& uidCache)
: PopSessionCommand(session),
  m_account(session.GetAccount()),
  m_uidMapPtr(session.GetUidMap()),
  m_localDeletedEmailUids(localDeletedEmailUids),
  m_uidCache(uidCache),
  m_uidNdx(0),
  m_deleteEmailResponseSlot(this, &DeleteServerEmailsCommand::DeleteEmailResponse),
  m_moveDeletedEmailResponseSlot(this, &DeleteServerEmailsCommand::MoveEmailToTrashFolderResponse)
{
}

DeleteServerEmailsCommand::~DeleteServerEmailsCommand()
{
}

void DeleteServerEmailsCommand::RunImpl()
{
	MojLogInfo(m_log, "Number of locally deleted emails to handle: %d", m_localDeletedEmailUids.size());
	DeleteNextEmail();
}

void DeleteServerEmailsCommand::DeleteNextEmail()
{
	if (m_uidNdx < (int)m_localDeletedEmailUids.size()) {
		m_currDeletedEmail = m_localDeletedEmailUids.at(m_uidNdx++);
		
		// make sure UID presents in the server list before deleting it from server
		if (m_uidMapPtr->HasUid(m_currDeletedEmail.m_uid) && m_account->IsDeleteFromServer()) {
			DeleteServerEmail();
		} else {
			// even though the UID doesn't exist, still try to move to email
			// to trash folder so that the same email won't be picked up in the
			// next round of inbox sync.
			MoveDeletedEmailToTrashFolder();
		}
	} else {
		Complete();
	}
}

void DeleteServerEmailsCommand::DeleteServerEmail()
{
	int msgNum = m_uidMapPtr->GetMessageNumber(m_currDeletedEmail.m_uid);
	MojLogInfo(m_log, "Deleting email uid='%s' msgNum='%d' from server", m_currDeletedEmail.m_uid.c_str(), msgNum);

	m_deleteEmailResponseSlot.cancel();
	m_delServerResult.reset(new PopCommandResult(m_deleteEmailResponseSlot));

	m_delServerEmailCommand.reset(new DeleCommand(m_session, msgNum));
	m_delServerEmailCommand->SetResult(m_delServerResult);
	m_delServerEmailCommand->Run();
}

MojErr DeleteServerEmailsCommand::DeleteEmailResponse()
{
	// moves deleted email to trash folder
	MoveDeletedEmailToTrashFolder();
	return MojErrNone;
}

void DeleteServerEmailsCommand::MoveDeletedEmailToTrashFolder()
{
	//moves deleted email from inbox to trash folder by first getting the deleted
	//email from database and then change its folder id to trash folder id
	//and also change "_del" property to false.
	try {
		if (m_currDeletedEmail.m_id.undefined()) {
			if (m_account->IsDeleteFromServer()) {
				// if deleted email id is undefined, that means the account preference
				// has been just changed "Sync deleted emails" flag from false to true.
				// In this case, we don't need to move the email since the email
				// is already in TRASH folder.
			} else {
				// this is move email case
				m_uidCache.GetDeletedEmailsCache().AddToLocalDeletedEmailCache(m_currDeletedEmail.m_uid);
				m_uidCache.GetDeletedEmailsCache().SetChanged(true);
			}

			DeleteNextEmail();
		} else {
			m_moveDeletedEmailResponseSlot.cancel();
			m_session.GetDatabaseInterface().MoveDeletedEmailToTrash(m_moveDeletedEmailResponseSlot, m_currDeletedEmail.m_id, m_account->GetTrashFolderId());
		}
	} catch (const std::exception& ex) {
		MojLogError(m_log, "Unable to move email to trash folder: %s", ex.what());
		DeleteNextEmail();
	} catch (...) {
		MojLogError(m_log, "Unknown exception in moving email to trash folder");
		DeleteNextEmail();
	}
}

MojErr DeleteServerEmailsCommand::MoveEmailToTrashFolderResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		if (!m_account->IsDeleteFromServer()) {
			// add uid to deleted emails cache if "Sync deleted emails" is turned off
			// so that we can have a record of which emails are deleted locally
			// to help next reconciliation.
			m_uidCache.GetDeletedEmailsCache().AddToLocalDeletedEmailCache(m_currDeletedEmail.m_uid);
			m_uidCache.GetDeletedEmailsCache().SetChanged(true);
		}
	} catch (const std::exception& ex) {
		MojLogError(m_log, "Unable to move email to trash folder: %s", ex.what());
	} catch (...) {
		MojLogError(m_log, "Unknown exception in response of moving email to trash folder");
	}

	DeleteNextEmail();
	return MojErrNone;
}
