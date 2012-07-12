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

#include "activity/ActivityParser.h"
#include "activity/ActivityBuilder.h"
#include "activity/ActivityBuilderFactory.h"
#include "commands/MoveEmailsCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailSchema.h"
#include "data/PopAccountAdapter.h"
#include "data/PopFolderAdapter.h"

MoveEmailsCommand::MoveEmailsCommand(PopClient& client, MojServiceMessage* msg, MojObject& payload)
: PopClientCommand(client, "Move POP outbox emails"),
  m_client(client),
  m_msg(msg),
  m_payload(payload),
  m_sentEmailsQuerySlot(this, &MoveEmailsCommand::SentEmailsQueryResponse),
  m_activityUpdateSlot(this, &MoveEmailsCommand::ActivityUpdate),
  m_activityErrorSlot(this, &MoveEmailsCommand::ActivityError),
  m_folderUpdateSlot(this, &MoveEmailsCommand::FolderUpdateResponse)
{

}

MoveEmailsCommand::~MoveEmailsCommand()
{
}

void MoveEmailsCommand::RunImpl()
{
	try {
		// query sendStatus.sent = true
		// set folder ID to sentFolderId, set flags.visible=true
		MojLogInfo(m_log, "MoveEmailsCommand::RunImpl");

		m_activity = ActivityParser::GetActivityFromPayload(m_payload);

		if (m_activity.get()) {
			m_activity->SetSlots(m_activityUpdateSlot, m_activityErrorSlot);
			m_activity->Adopt(m_client);
			return;
		} else {
			SentEmailsQuery();
		}
	} catch (const std::exception& ex) {
		m_msg->replyError(MojErrInternal, ex.what());
		Failure(ex);
	} catch (...) {
		MailException ex("unknown exception", __FILE__, __LINE__);
		m_msg->replyError(MojErrInternal, ex.what());
		Failure(ex);
	}
}

void MoveEmailsCommand::SentEmailsQuery()
{
	// exceptions in this function will be handled by caller
	MojObject outboxFolderId;
	MojErr err = m_payload.getRequired(PopFolderAdapter::OUTBOX_FOLDER_ID, outboxFolderId);
	ErrorToException(err);

	m_client.GetDatabaseInterface().GetSentEmails(m_sentEmailsQuerySlot, outboxFolderId, 1);
}

MojErr MoveEmailsCommand::ActivityUpdate(Activity* activity, Activity::EventType event)
{
	try {
		switch (event) {
		case Activity::StartEvent:
			SentEmailsQuery();
			break;
		case Activity::CompleteEvent:
			m_activityUpdateSlot.cancel();
			m_activityErrorSlot.cancel();
			Complete();
			break;
		default:
			// do nothing
			break;
		}
	} catch (const std::exception& ex) {
		MojLogInfo(m_log, "could not make MoveEmailsCommand DB query");
		m_msg->replyError(MojErrInternal, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

MojErr MoveEmailsCommand::ActivityError(Activity* activity, Activity::ErrorType error, const std::exception& exc)
{
	m_msg->replyError(MojErrInternal);
	Failure(exc);

	return MojErrNone;
}

MojErr MoveEmailsCommand::SentEmailsQueryResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);

		MojObject email;
		results.at(0, email);

		MojObject id;
		err = email.getRequired(DatabaseAdapter::ID, id);
		ErrorToException(err);
		MojObject folderId;
		err = m_payload.getRequired(EmailAccountAdapter::SENT_FOLDERID, folderId);
		ErrorToException(err);
		MojObject flags;
		err = email.getRequired("flags", flags);
		ErrorToException(err);

		// set to be read
		err = flags.put("read", true);
		ErrorToException(err);
		err = flags.put("visible", true);
		ErrorToException(err);

		MojObject folderUpdate;
		err = folderUpdate.put(DatabaseAdapter::ID, id);
		ErrorToException(err);
		err = folderUpdate.put("folderId", folderId);
		ErrorToException(err);
		err = folderUpdate.put("flags", flags);
		ErrorToException(err);

		m_client.GetDatabaseInterface().UpdateItem(m_folderUpdateSlot, folderUpdate);
	} catch (const std::exception& ex) {
		m_msg->replyError(err, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

MojErr MoveEmailsCommand::FolderUpdateResponse(MojObject& response, MojErr err)
{
	try {
		// get outboxFolderId
		MojObject outboxFolderId;
		err = m_payload.getRequired(EmailAccountAdapter::OUTBOX_FOLDERID, outboxFolderId);
		ErrorToException(err);

		// get sentFolderId
		MojObject sentFolderId;
		err = m_payload.getRequired(EmailAccountAdapter::SENT_FOLDERID, sentFolderId);
		ErrorToException(err);

		// activity to setup watch
		ActivityBuilder ab;
		m_client.GetActivityBuilderFactory()->BuildSentEmailsWatch(ab, outboxFolderId, sentFolderId);
		m_activity->UpdateAndComplete(m_client, ab.GetActivityObject());

		m_msg->replySuccess();
		Complete();
	} catch (const std::exception& ex) {
		Failure(ex);
		m_msg->replyError(err, ex.what());
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}
