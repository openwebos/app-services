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

#include "PopDefs.h"
#include "commands/PopAccountDisableCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/UidCacheAdapter.h"
#include "data/PopFolderAdapter.h"
#include "data/PopEmailAdapter.h"
#include "data/PopAccountAdapter.h"
#include <string>
#include <vector>

PopAccountDisableCommand::PopAccountDisableCommand(PopClient& client, boost::shared_ptr<DatabaseInterface> dbInterface,
														   const MojObject& payload, MojRefCountedPtr<MojServiceMessage> msg)
: PopClientCommand(client, "Disable POP account", Command::LowPriority),
  m_log("com.palm.pop"),
  m_client(client),
  m_dbInterface(dbInterface),
  m_payload(payload),
  m_getPopAccountFoldersSlot(this, &PopAccountDisableCommand::GetPopAccountFoldersResponse),
  m_deleteEmailsSlot(this, &PopAccountDisableCommand::DeleteEmailsResponse),
  m_deletePopFoldersSlot(this, &PopAccountDisableCommand::DeletePopFoldersResponse),
  m_deleteOldEmailsCacheSlot(this, &PopAccountDisableCommand::DeleteOldEmailsCacheResponse),
  m_getActivitiesSlot(this, &PopAccountDisableCommand::GetActivitiesResponse),
  m_cancelActivitiesSlot(this, &PopAccountDisableCommand::CancelActivitiesResponse),
  m_smtpAccountDisabledSlot(this, &PopAccountDisableCommand::SmtpAccountDisabledResponse),
  m_msg(msg)
{
}

PopAccountDisableCommand::~PopAccountDisableCommand()
{
}

void PopAccountDisableCommand::RunImpl()
{
	CommandTraceFunction();
	MojErr err = m_payload.getRequired("accountId", m_accountId);
	ErrorToException(err);
	m_dbInterface->GetAccountFolders(m_getPopAccountFoldersSlot, m_accountId);
}

MojErr PopAccountDisableCommand::GetPopAccountFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	MojString json;
	response.toJson(json);
	MojLogDebug(m_log, "PopClient::GetPopAccountFoldersResponse \n %s", json.data());

	try {
		ErrorToException(err);

		err = response.getRequired("results", m_popFolders);
		ErrorToException(err);

		if (!m_popFolders.empty()) {
			for (MojObject::ConstArrayIterator it = m_popFolders.arrayBegin(); it != m_popFolders.arrayEnd(); it++) {
				MojObject folderId;

				err = it->getRequired(DatabaseAdapter::ID, folderId);
				ErrorToException(err);

				err = m_popFolderIds.push(folderId);
				ErrorToException(err);
			}

			DeleteEmails();
		} else {
			DeleteOldEmailsCache();
		}


	} catch (const std::exception& e) {
		m_msg->replyError(err);
		Failure(e);
	} catch (...) {
		MojString error;
		error.format("uncaught exception in %s", __PRETTY_FUNCTION__);
		m_msg->replyError(MojErrInternal, error.data());
		Failure(MailException("unknown exception in gettting account folders response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountDisableCommand::DeleteEmails()
{
	CommandTraceFunction();

	// TODO: delete emails/attachments from file cache
	m_dbInterface->DeleteItems(m_deleteEmailsSlot, PopEmailAdapter::POP_EMAIL_KIND, "folderId", m_popFolderIds);
}

MojErr PopAccountDisableCommand::DeleteEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	MojString s;
	response.toJson(s);
	MojLogDebug(m_log, "%s", s.data());

	try {
		ErrorToException(err);
		DeletePopFolders();
	} catch (const std::exception& e) {
		m_msg->replyError(err);
		Failure(e);
	} catch (...) {
		MojString error;
		error.format("uncaught exception in %s", __PRETTY_FUNCTION__);
		m_msg->replyError(MojErrInternal, error.data());
		Failure(MailException("unknown exception in deleting emails response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountDisableCommand::DeletePopFolders()
{
	CommandTraceFunction();

	m_dbInterface->DeleteItems(m_deletePopFoldersSlot, PopFolderAdapter::POP_FOLDER_KIND, PopFolderAdapter::ID, m_popFolderIds);
}

MojErr PopAccountDisableCommand::DeletePopFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check response error
		ErrorToException(err);

		DeleteOldEmailsCache();
	} catch (const std::exception& e) {
		m_msg->replyError(MojErrInternal, e.what());
		Failure(e);
	} catch (...) {
		MojString error;
		error.format("uncaught exception in %s", __PRETTY_FUNCTION__);
		m_msg->replyError(MojErrInternal, error.data());
		Failure(MailException("unknown exception in deleting folder response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountDisableCommand::DeleteOldEmailsCache()
{
	CommandTraceFunction();

	m_dbInterface->DeleteItems(m_deleteOldEmailsCacheSlot, UidCacheAdapter::EMAILS_UID_CACHE_KIND, UidCacheAdapter::ACCOUNT_ID, m_accountId);
}

MojErr PopAccountDisableCommand::DeleteOldEmailsCacheResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check response error
		ErrorToException(err);
		GetActivities();
	} catch (const std::exception& e) {
		m_msg->replyError(MojErrInternal, e.what());
		Failure(e);
	} catch (...) {
		MojString error;
		error.format("uncaught exception in %s", __PRETTY_FUNCTION__);
		m_msg->replyError(MojErrInternal, error.data());
		Failure(MailException("unknown exception in deleting old emails cache response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountDisableCommand::GetActivities()
{
	CommandTraceFunction();

	m_client.SendRequest(m_getActivitiesSlot, "com.palm.activitymanager", "list", MojObject());
}

MojErr PopAccountDisableCommand::GetActivitiesResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check response error
		ErrorToException(err);

		MojObject activities;
		err = response.getRequired("activities", activities);
		ErrorToException(err);

		for (MojObject::ConstArrayIterator it = activities.arrayBegin(); it != activities.arrayEnd(); it++) {
			MojObject creator;
			(void) it->getRequired("creator", creator);

			MojString serviceId;
			(void) creator.getRequired("serviceId", serviceId);

			// make sure we're the creator of the activity
			if (serviceId == "com.palm.pop") {
				MojObject metadata;
				bool metadataExists = it->get("metadata", metadata);

				if (metadataExists) {
					MojObject accountId;
					bool accountIdExists = metadata.get("accountId", accountId);

					if (accountIdExists) {
						if (accountId == m_client.GetAccountId()) {
							MojObject activityId;
							err = it->getRequired("activityId", activityId);
							ErrorToException(err);
							m_activitiesList.push_back(activityId);

							MojString json;
							err = activityId.toJson(json);
							ErrorToException(err);
							MojLogInfo(m_log, "activity %s queued up for cancellation", json.data());
						}
					}
				}
			}
		}

		CancelActivities();
	} catch (const std::exception& e) {
		m_msg->replyError(MojErrInternal, e.what());
		Failure(e);
	} catch (...) {
		MojString error;
		error.format("uncaught exception in %s", __PRETTY_FUNCTION__);
		m_msg->replyError(MojErrInternal, error.data());
		Failure(MailException("unknown exception in getting activity response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountDisableCommand::CancelActivities()
{
	CommandTraceFunction();

	if(!m_activitiesList.empty()) {
		MojErr err;
		MojObject payload;

		MojObject activityId = m_activitiesList.back();
		m_activitiesList.pop_back();

		err = payload.put("activityId", activityId);
		ErrorToException(err);

		MojLogInfo(m_log, "Canceling activity %s", AsJsonString(activityId).c_str());
		m_cancelActivitiesSlot.cancel();
		m_client.SendRequest(m_cancelActivitiesSlot, "com.palm.activitymanager", "cancel", payload);
	}
	else {
		MojLogInfo(m_log, "Disabling SMTP account");
		m_client.SendRequest(m_smtpAccountDisabledSlot, "com.palm.smtp", "accountEnabled", m_payload);
	}
}

MojErr PopAccountDisableCommand::CancelActivitiesResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	try {
		ErrorToException(err);

		MojString json;
		err = response.toJson(json);
		MojLogDebug(m_log, "CancelActivitiesResponse: %s", json.data());

		CancelActivities();
	} catch (const std::exception& e) {
		m_msg->replyError(err, e.what());
		Failure(e);
	} catch (...) {
		MojString error;
		error.format("uncaught exception in %s", __PRETTY_FUNCTION__);
		m_msg->replyError(MojErrInternal, error.data());
		Failure(MailException("unknown exception in cancelling activities response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

MojErr PopAccountDisableCommand::SmtpAccountDisabledResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		m_msg->replySuccess();
		Complete();
	} catch (const std::exception& e) {
		m_msg->replyError(err, e.what());
		Failure(e);
	} catch (...) {
		MojString error;
		error.format("uncaught exception in %s", __PRETTY_FUNCTION__);
		m_msg->replyError(MojErrInternal, error.data());
		Failure(MailException("unknown exception in disabling SMTP response", __FILE__, __LINE__));
	}

	return MojErrNone;
}
