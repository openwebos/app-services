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

#include "commands/AutoDownloadCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/PopEmailAdapter.h"
#include "exceptions/MailException.h"
#include "PopDefs.h"

AutoDownloadCommand::AutoDownloadCommand(PopSession& session, const MojObject& folderId)
: PopSessionPowerCommand(session, "Auto download emails"),
  m_folderId(folderId),
  m_lastSyncRev(0),
  m_numEmailsExamined(0),
  m_maxBodies(MAX_EMAIL_BODIES),
  m_numDownloadRequests(0),
  m_getEmailsToFetchResponseSlot(this, &AutoDownloadCommand::GetEmailsToFetchResponse)
{

}

AutoDownloadCommand::~AutoDownloadCommand()
{

}

void AutoDownloadCommand::RunImpl()
{
	MojLogInfo(m_log, "Loading folder '%s' emails to fetch bodies", AsJsonString(m_folderId).c_str());
	m_session.GetSyncSession()->RegisterCommand(this);
	GetEmailsToFetch();
}

void AutoDownloadCommand::GetEmailsToFetch()
{
	m_getEmailsToFetchResponseSlot.cancel();  // in case this is called multiple times
	m_session.GetDatabaseInterface().GetAutoDownloadEmails(m_getEmailsToFetchResponseSlot, m_folderId, m_lastSyncRev, m_emailsPage, BATCH_SIZE);
}

MojErr AutoDownloadCommand::GetEmailsToFetchResponse(MojObject& response, MojErr err)
{

	try {
		// Check database response
		ErrorToException(err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		MojObject::ArrayIterator it;
		err = results.arrayBegin(it);
		ErrorToException(err);

		for (; it != results.arrayEnd(); ++it) {
			MojObject& emailObj = *it;
			MojErr err;

			m_numEmailsExamined++;

			if (m_numEmailsExamined > m_maxBodies && m_maxBodies >= 0) {
				break;
			}

			MojObject id;
			err = emailObj.getRequired(PopEmailAdapter::ID, id);
			ErrorToException(err);

			bool downloaded;
			err = emailObj.getRequired(PopEmailAdapter::DOWNLOADED, downloaded);
			ErrorToException(err);

			if (!downloaded) {
				MojLogDebug(m_log, "Fetching email from pop session");
				m_session.FetchEmail(id, MojObject::Null, m_listener, true);

				m_numDownloadRequests++;
			}
		}

		if (DatabaseAdapter::GetNextPage(response, m_emailsPage) && (m_numEmailsExamined < m_maxBodies || m_maxBodies < 0)) {
			GetEmailsToFetch();
		} else {
			Complete();
		}
	} catch(const std::exception& e) {
		MojLogError(m_log, "Unable to get auto download emails: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to get auto download emails", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}


void AutoDownloadCommand::Complete()
{
	if (m_numDownloadRequests > 0) {
		MojLogInfo(m_log, "queued a total of %d download requests", m_numDownloadRequests);
	}

	m_session.GetSyncSession()->CommandCompleted(this);
	PopSessionPowerCommand::Complete();
}

void AutoDownloadCommand::Failure(const std::exception& ex)
{
	m_session.GetSyncSession()->CommandFailed(this, ex);
	PopSessionPowerCommand::Failure(ex);
}

