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

#include "commands/MovePopEmailsCommand.h"
#include "activity/ActivityParser.h"
#include "data/PopEmailAdapter.h"

MovePopEmailsCommand::MovePopEmailsCommand(PopClient& client, MojServiceMessage* msg, MojObject& payload)
: PopClientCommand(client, "Move single POP email"),
  m_msg(msg),
  m_payload(payload),
  m_activityUpdateSlot(this, &MovePopEmailsCommand::ActivityUpdate),
  m_activityErrorSlot(this, &MovePopEmailsCommand::ActivityError),
  m_getEmailsToMoveSlot(this, &MovePopEmailsCommand::GetEmailsToMoveResponse),
  m_emailsMovedSlot(this, &MovePopEmailsCommand::EmailsMovedResponse),
  m_getUidCacheResponseSlot(this, &MovePopEmailsCommand::GetUidCacheResponse),
  m_saveUidCacheResponseSlot(this, &MovePopEmailsCommand::SaveUidCacheResponse)
{

}

MovePopEmailsCommand::~MovePopEmailsCommand()
{

}

void MovePopEmailsCommand::RunImpl()
{
	CommandTraceFunction();
	try {

		if (!m_client.GetAccount().get()) {
			MojString err;
			err.format("Account is not loaded for '%s'", AsJsonString(
					m_client.GetAccountId()).c_str());
			throw MailException(err.data(), __FILE__, __LINE__);
		}

		MojErr err = m_payload.getRequired("accountId", m_accountId);
		ErrorToException(err);

		m_activity = ActivityParser::GetActivityFromPayload(m_payload);

		if (m_activity.get()) {
			m_activity->SetSlots(m_activityUpdateSlot, m_activityErrorSlot);
			m_activity->Adopt(m_client);
			return;
		} else {
			GetEmailsToMove();
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

MojErr MovePopEmailsCommand::ActivityUpdate(Activity* activity, Activity::EventType event)
{
	try {
		switch (event) {
		case Activity::StartEvent:
			GetEmailsToMove();
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
		m_msg->replyError(MojErrInternal, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

MojErr MovePopEmailsCommand::ActivityError(Activity* activity, Activity::ErrorType error, const std::exception& exc)
{
	m_msg->replyError(MojErrInternal);
	Failure(exc);

	return MojErrNone;
}

void MovePopEmailsCommand::GetEmailsToMove()
{
	m_client.GetDatabaseInterface().GetEmailsToMove(m_getEmailsToMoveSlot, m_accountId);
}

MojErr MovePopEmailsCommand::GetEmailsToMoveResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojLogInfo(m_log, "response to get emails to move: %s", AsJsonString(response).c_str());

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);

		MojObject::ObjectVec movedEmails;
		PopClient::AccountPtr account = m_client.GetAccount();

		for (MojObject::ConstArrayIterator it = results.arrayBegin(); it != results.arrayEnd(); it++) {
			MojObject singleMovedEmail;

			MojObject id;
			err = it->getRequired(PopEmailAdapter::ID, id);
			ErrorToException(err);
			MojObject srcFolderId;
			err = it->getRequired(EmailSchema::FOLDER_ID, srcFolderId);
			ErrorToException(err);
			MojObject destFolderId;
			err = it->getRequired("destFolderId", destFolderId);
			ErrorToException(err);
			MojObject flags;
			err = it->getRequired("flags", flags);
			ErrorToException(err);

			// set to be visible
			err = flags.put("visible", true);
			ErrorToException(err);

			// setup moved email, push onto moved emails vector
			err = singleMovedEmail.put(PopEmailAdapter::ID, id);
			ErrorToException(err);
			err = singleMovedEmail.put("folderId", destFolderId);
			ErrorToException(err);
			err = singleMovedEmail.put("destFolderId", MojObject());
			ErrorToException(err);
			err = singleMovedEmail.put("flags", flags);
			ErrorToException(err);
			// add emails to be moved into 'movedEmails' list
			movedEmails.push(singleMovedEmail);

			if (account->GetInboxFolderId() == srcFolderId) {
				// add moved inbox emails inot 'm_inboxEmailsMoved' set
				MojString uid;
				err = it->getRequired(PopEmailAdapter::SERVER_UID, uid);
				ErrorToException(err);

				m_inboxEmailsMoved.insert(std::string(uid));
			}
		}

		m_client.GetDatabaseInterface().UpdateItems(m_emailsMovedSlot, movedEmails);
	} catch (const std::exception& ex) {
		m_msg->replyError(MojErrInternal, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

MojErr MovePopEmailsCommand::EmailsMovedResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		ActivityBuilder ab;
		m_client.GetActivityBuilderFactory()->BuildMoveEmailsWatch(ab);
		m_activity->UpdateAndComplete(m_client, ab.GetActivityObject());

		m_msg->replySuccess();

		if (m_inboxEmailsMoved.size() > 0) {
			// add the list of UIDs into UidCache
			GetUidCache();
		} else {
			Complete();
		}
	} catch (const std::exception& ex) {
		m_msg->replyError(MojErrInternal, ex.what());
		Failure(ex);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void MovePopEmailsCommand::GetUidCache()
{
	m_getUidCacheResponseSlot.cancel();
	m_commandResult.reset(new PopCommandResult(m_getUidCacheResponseSlot));
	m_loadCacheCommand.reset(new LoadUidCacheCommand(*m_client.GetSession().get(), m_accountId, m_uidCache));
	m_loadCacheCommand->SetResult(m_commandResult);
	m_loadCacheCommand->Run();
}


MojErr MovePopEmailsCommand::GetUidCacheResponse()
{
	try {
		m_commandResult->CheckException();

		std::set<std::string>::iterator itr = m_inboxEmailsMoved.begin();
		while (itr != m_inboxEmailsMoved.end()) {
			std::string uid = *itr;
			m_uidCache.GetDeletedEmailsCache().AddToPendingDeletedEmailCache(uid);
			itr++;
		}
		m_uidCache.GetDeletedEmailsCache().SetChanged(true);

		SaveUidCache();
	} catch(const std::exception& e) {
		MojLogError(m_log, "Failed to sync folder: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to get old emails cache", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}

void MovePopEmailsCommand::SaveUidCache()
{
	m_saveUidCacheResponseSlot.cancel();
	m_commandResult.reset(new PopCommandResult(m_saveUidCacheResponseSlot));
	m_saveCacheCommand.reset(new UpdateUidCacheCommand(*m_client.GetSession().get(), m_uidCache));
	m_saveCacheCommand->SetResult(m_commandResult);
	m_saveCacheCommand->Run();
}


MojErr MovePopEmailsCommand::SaveUidCacheResponse()
{
	try {
		m_commandResult->CheckException();
		Complete();
	} catch(const std::exception& e) {
		MojLogError(m_log, "Failed to sync folder: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to get old emails cache", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}
