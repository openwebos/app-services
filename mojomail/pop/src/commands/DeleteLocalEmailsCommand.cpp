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

#include "commands/DeleteLocalEmailsCommand.h"
#include "data/PopEmailAdapter.h"
#include "PopDefs.h"

DeleteLocalEmailsCommand::DeleteLocalEmailsCommand(PopSession& session,
		const MojObject::ObjectVec& serverDeletedEmailIds,
		const MojObject::ObjectVec& oldEmailIds)
: PopSessionCommand(session),
  m_account(session.GetAccount()),
  m_serverDeletedEmailIds(serverDeletedEmailIds),
  m_oldEmailIds(oldEmailIds),
  m_deleteLocalEmailsResponseSlot(this, &DeleteLocalEmailsCommand::DeleteLocalEmailsResponse)
{
	if (m_account->IsDeleteOnDevice()) {
		MojLogInfo(m_log, "Number of emails to be deleted locally: %d", (int)m_serverDeletedEmailIds.size());
	}
}

DeleteLocalEmailsCommand::~DeleteLocalEmailsCommand()
{
}

void DeleteLocalEmailsCommand::RunImpl()
{
	DetermineDeletedEmails();
	DeleteLocalEmails(m_deletedEmailIds);
}

void DeleteLocalEmailsCommand::DetermineDeletedEmails()
{
	// First, add the emails that are beyond sync windows into deleted emails list
	m_deletedEmailIds.append(m_oldEmailIds.begin(), m_oldEmailIds.end());

	// Second, add server deleted emails into deleted emails list if 'deleteOnDevice' flag is turned on
	if (m_account->IsDeleteOnDevice()) {
		m_deletedEmailIds.append(m_serverDeletedEmailIds.begin(), m_serverDeletedEmailIds.end());
	}
}

void DeleteLocalEmailsCommand::DeleteLocalEmails(const MojObject::ObjectVec& delEmails)
{
	if (delEmails.size() > 0) {
		//MojLogInfo(m_log, "Deleting %d email(s) from database", delEmails.size());
		m_session.GetDatabaseInterface().DeleteItems(m_deleteLocalEmailsResponseSlot, delEmails);
	} else {
		Complete();
	}
}

MojErr DeleteLocalEmailsCommand::DeleteLocalEmailsResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);
		Complete();
	} catch(const std::exception& e) {
		MojLogError(m_log, "Unable to delete local emails: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to delete local emails", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}
