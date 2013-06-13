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

#include "commands/InsertEmailsCommand.h"
#include "data/PopEmailAdapter.h"

InsertEmailsCommand::InsertEmailsCommand(PopSession& session, PopEmail::PopEmailPtrVectorPtr emails)
: PopSessionCommand(session),
  m_emails(emails),
  m_reserveIdsSlot(this, &InsertEmailsCommand::ReserverEmailIdsResponse),
  m_saveEmailsSlot(this, &InsertEmailsCommand::SaveEmailsResponse)
{
}

InsertEmailsCommand::~InsertEmailsCommand()
{
}

void InsertEmailsCommand::RunImpl()
{
	m_session.GetDatabaseInterface().ReserveIds(m_reserveIdsSlot, m_emails->size());
}

MojErr InsertEmailsCommand::ReserverEmailIdsResponse(MojObject& response, MojErr err)
{
	try
	{
		ErrorToException(err);

		MojLogDebug(m_log, "Got reserver ids response: %s", AsJsonString(response).c_str());
		MojObject idArray;
		err = response.getRequired("ids", idArray);
		ErrorToException(err);

		for (int ndx = 0; ndx < (int)m_emails->size(); ndx++) {
			MojObject id;
			idArray.at(ndx, id);

			PopEmail::PopEmailPtr emailPtr = m_emails->at(ndx);
			MojLogDebug(m_log, "Assigning id '%s' to email '%s'", AsJsonString(id).c_str(), emailPtr->GetServerUID().c_str());
			emailPtr->SetId(id);
		}

		SaveEmails();
	}
	catch (const std::exception& e) {
		MojLogError(m_log, "Exception in reserving email ID: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in reserving email ID");
		Failure(MailException("Unknown exception in reserving email ID", __FILE__, __LINE__));
		
	}

	return MojErrNone;
}

MojErr InsertEmailsCommand::SaveEmails()
{
	try{
		for (PopEmail::PopEmailPtrVector::iterator itr = m_emails->begin(); itr != m_emails->end(); itr++) {
			PopEmail::PopEmailPtr emailPtr = *itr;
			MojObject mojEmail;
			PopEmailAdapter::SerializeToDatabasePopObject(*emailPtr, mojEmail);
			MojErr err = m_persistEmails.push(mojEmail);
			ErrorToException(err);
		}

		MojLogInfo(m_log, "Adding %d email(s) to database", m_emails->size());
		m_session.GetDatabaseInterface().AddItems(m_saveEmailsSlot, m_persistEmails);
	}
	catch (const std::exception& e) {
		MojLogError(m_log, "Exception in save emails: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in save emails");
		Failure(MailException("Unknown exception in save emails", __FILE__, __LINE__));
	}
	return MojErrNone;
}

MojErr InsertEmailsCommand::SaveEmailsResponse(MojObject& response, MojErr err)
{
	MojErrCheck(err);

	// TODO: get revision from insert emails response

	Complete();

	return MojErrNone;
}
