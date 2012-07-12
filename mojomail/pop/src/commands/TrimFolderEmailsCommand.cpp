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

#include "PopConfig.h"
#include "commands/TrimFolderEmailsCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailSchema.h"
#include "data/PopEmailAdapter.h"

const int TrimFolderEmailsCommand::LOAD_EMAIL_BATCH_SIZE 	= 10;

TrimFolderEmailsCommand::TrimFolderEmailsCommand(PopSession& session,
		const MojObject& folderId, int trimCount, UidCache& uidCache)
: PopSessionCommand(session),
  m_folderId(folderId),
  m_numberToTrim(trimCount),
  m_uidCache(uidCache),
  m_getLocalEmailsResponseSlot(this, &TrimFolderEmailsCommand::GetLocalEmailsResponse),
  m_deleteLocalEmailsResponseSlot(this, &TrimFolderEmailsCommand::DeleteLocalEmailsResponse)
{

}

TrimFolderEmailsCommand::~TrimFolderEmailsCommand()
{

}

void TrimFolderEmailsCommand::RunImpl()
{
	if (m_numberToTrim > 0) {
		MojLogInfo(m_log, "Need to trim %d emails to maintain the folder to have at most %d emails", m_numberToTrim, PopConfig::MAX_EMAIL_COUNT_ON_DEVICE);
		GetLocalEmails();
	} else {
		Complete();
	}
}

void TrimFolderEmailsCommand::GetLocalEmails()
{
	MojInt32 lastRev = 0;
	MojLogInfo(m_log, "Loading local emails");
	m_getLocalEmailsResponseSlot.cancel();  // in case this is called multiple times
	// retrieve emails in the order from oldest to newest so that we can push out old emails first
	bool descending = false;
	m_session.GetDatabaseInterface().GetEmailSyncList(m_getLocalEmailsResponseSlot, m_folderId, lastRev, descending, m_localEmailsPage, LOAD_EMAIL_BATCH_SIZE);
}

MojErr TrimFolderEmailsCommand::GetLocalEmailsResponse(MojObject& response, MojErr err)
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

			MojObject id;
			err = emailObj.getRequired(PopEmailAdapter::ID, id);
			ErrorToException(err);

			MojString uid;
			err = emailObj.getRequired(PopEmailAdapter::SERVER_UID, uid);
			ErrorToException(err);
			std::string uidStr(uid);

			MojInt64 timestamp;
			err = emailObj.getRequired(EmailSchema::TIMESTAMP, timestamp);

			// add email ID to the list of IDs to be deleted
			m_idsToTrim.push(id);
			// move trimmed emails to old emails' cache so that they won't be
			// added in the next sync
			m_uidCache.GetOldEmailsCache().AddToCache(uid.data(), timestamp);
			m_uidCache.GetOldEmailsCache().SetChanged(true);

			// decrement the number of emails to be trimmed
			if (--m_numberToTrim == 0) {
				break;
			}
		}

		bool hasMore = DatabaseAdapter::GetNextPage(response, m_localEmailsPage);
		if(m_numberToTrim > 0 && hasMore) {
			// Get next batch of emails to be deleted
			MojLogInfo(m_log, "getting another batch of local emails");
			GetLocalEmails();
		} else {
			MojLogInfo(m_log, "Delete local emails cache");
			DeleteLocalEmails();
		}
	} catch(const std::exception& e) {
		MojLogError(m_log, "Failed to get local emails: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to get local emails", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}

void TrimFolderEmailsCommand::DeleteLocalEmails()
{
	if (m_idsToTrim.size() > 0)
	{
		MojLogInfo(m_log, "Deleting %d email(s) from database", m_idsToTrim.size());
		m_session.GetDatabaseInterface().DeleteItems(m_deleteLocalEmailsResponseSlot, m_idsToTrim);
	} else {
		Complete();
	}
}

MojErr TrimFolderEmailsCommand::DeleteLocalEmailsResponse(MojObject& response, MojErr err)
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
