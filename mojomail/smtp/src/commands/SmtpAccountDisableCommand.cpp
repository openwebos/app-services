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

#include "commands/SmtpAccountDisableCommand.h"
#include "SmtpClient.h"
#include "data/EmailSchema.h"
#include <boost/regex.hpp>
#include "data/SmtpAccountAdapter.h"
#include "activity/ActivityBuilder.h"
#include "client/BusClient.h"

static const char* OUTBOX_WATCH_ACTIVITY_FMT = "SMTP Watch/accountId=\"%s\"";
static const char* ACCOUNT_WATCH_ACTIVITY_FMT = "SMTP Account Watch/accountId=\"%s\"";

// TODO: Need to use ActivitySet for activity handling.
SmtpAccountDisableCommand::SmtpAccountDisableCommand(SmtpClient& client, const MojRefCountedPtr<MojServiceMessage> msg, const MojObject& accountId)
: SmtpCommand(client),
  m_client(client),
  m_accountId(accountId),
  m_deleteOutboxWatchResponseSlot(this, &SmtpAccountDisableCommand::DeleteOutboxWatchResponse),
  m_deleteAccountWatchResponseSlot(this, &SmtpAccountDisableCommand::DeleteAccountWatchResponse),
  m_msg(msg)
{
	MojString d;
	m_accountId.stringValue(d);
	m_outboxWatchActivityName.format(OUTBOX_WATCH_ACTIVITY_FMT, d.data());
	m_accountWatchActivityName.format(ACCOUNT_WATCH_ACTIVITY_FMT, d.data());
}

SmtpAccountDisableCommand::~SmtpAccountDisableCommand()
{
}

void SmtpAccountDisableCommand::RunImpl()
{
	MojLogInfo(m_log, "SmtpAccountDisable clearing session account");
	
	m_client.GetSession()->ClearAccount();
	
	MojLogInfo(m_log, "SmtpAccountDisable removing watch");
	
	MojObject payload;
	
	payload.put("activityName", m_outboxWatchActivityName);

	MojString json;
	payload.toJson(json);

	MojLogInfo(m_log, "SmtpAccountDisable payload %s", json.data());

	m_client.SendRequest(m_deleteOutboxWatchResponseSlot, "com.palm.activitymanager", "cancel", payload);
}

MojErr SmtpAccountDisableCommand::DeleteOutboxWatchResponse(MojObject& response, MojErr err)
{
	MojLogInfo(m_log, "SmtpAccountDisable removing outbox watch");

	try {
		if(err) {
			if(err == ENOENT) {
				MojLogWarning(m_log, "outbox watch activity doesn't exist");
			} else {
				ResponseToException(response, err);
			}
		}

		MojObject payload;
		payload.put("activityName", m_accountWatchActivityName);

		m_client.SendRequest(m_deleteAccountWatchResponseSlot, "com.palm.activitymanager", "cancel", payload);
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

MojErr SmtpAccountDisableCommand::DeleteAccountWatchResponse(MojObject& response, MojErr err)
{
	MojLogInfo(m_log, "SmtpAccountDisable replying");

	try {
		if(err) {
			if(err == ENOENT) {
				MojLogWarning(m_log, "account watch activity doesn't exist");
			} else {
				ResponseToException(response, err);
			}
		}

		m_msg->replySuccess();
		Complete();
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
