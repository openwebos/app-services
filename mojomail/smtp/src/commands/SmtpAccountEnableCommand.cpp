// @@@LICENSE
//
//      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

#include "commands/SmtpAccountEnableCommand.h"
#include "SmtpClient.h"
#include "core/MojServiceRequest.h"
#include "activity/ActivityBuilder.h"
#include "activity/SmtpActivityFactory.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailAccountAdapter.h"

// TODO: Need to use ActivitySet for activity handling.
SmtpAccountEnableCommand::SmtpAccountEnableCommand(SmtpClient& client, const MojRefCountedPtr<MojServiceMessage>& msg)
: SmtpCommand(client),
  m_client(client),
  m_msg(msg),
  m_accountRev(0),
  m_getAccountSlot(this, &SmtpAccountEnableCommand::GetMailAccountResponse),
  m_createSmtpConfigWatchSlot(this, &SmtpAccountEnableCommand::CreateSmtpConfigWatchResponse),
  m_createOutboxWatchSlot(this, &SmtpAccountEnableCommand::CreateOutboxWatchResponse)
{
}

SmtpAccountEnableCommand::~SmtpAccountEnableCommand()
{
}

void SmtpAccountEnableCommand::RunImpl()
{
	GetMailAccount();
}

void SmtpAccountEnableCommand::GetMailAccount()
{
	// Get the com.palm.mail.account so we can find the outbox id
	// FIXME merge this logic with SmtpSyncOutboxCommand or SmtpClient
	m_client.GetDatabaseInterface().GetAccount(m_getAccountSlot, m_client.GetAccountId());
}

MojErr SmtpAccountEnableCommand::GetMailAccountResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojObject account;

		DatabaseAdapter::GetOneResult(response, account);

		err = account.getRequired(DatabaseAdapter::REV, m_accountRev);
		ErrorToException(err);

		err = account.getRequired(EmailAccountAdapter::OUTBOX_FOLDERID, m_outboxFolderId);
		ErrorToException(err);

		CreateSmtpConfigWatch();
	} catch(const std::exception& e) {
		Failure(e);
	}

	return MojErrNone;
}

void SmtpAccountEnableCommand::CreateSmtpConfigWatch()
{
	SmtpActivityFactory factory;
	ActivityBuilder ab;

	factory.BuildSmtpConfigWatch(ab, m_client.GetAccountId(), m_accountRev);

	// Create payload
	MojObject payload;
	MojErr err;

	err = payload.put("activity", ab.GetActivityObject());
	ErrorToException(err);
	err = payload.put("start", true);
	ErrorToException(err);
	err = payload.put("replace", true);
	ErrorToException(err);

	m_client.SendRequest(m_createSmtpConfigWatchSlot, "com.palm.activitymanager", "create", payload);
}

MojErr SmtpAccountEnableCommand::CreateSmtpConfigWatchResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		CreateOutboxWatch();
	} catch(const std::exception& e) {
		Failure(e);
	}

	return MojErrNone;
}

void SmtpAccountEnableCommand::CreateOutboxWatch()
{
	SmtpActivityFactory factory;
	ActivityBuilder ab;

	factory.BuildOutboxWatch(ab, m_client.GetAccountId(), m_outboxFolderId);

	// Create payload
	MojObject payload;
	MojErr err;

	err = payload.put("activity", ab.GetActivityObject());
	ErrorToException(err);
	err = payload.put("start", true);
	ErrorToException(err);
	err = payload.put("replace", true);
	ErrorToException(err);

	m_client.SendRequest(m_createOutboxWatchSlot, "com.palm.activitymanager", "create", payload);
}

MojErr SmtpAccountEnableCommand::CreateOutboxWatchResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		Done();
	} catch(const std::exception& e) {
		Failure(e);
	}

	return MojErrNone;
}

void SmtpAccountEnableCommand::Failure(const std::exception& e)
{
	if(m_msg.get())
		m_msg->replyError(MojErrInternal, e.what());

	MojLogError(m_log, "error enabling SMTP for account %s: %s", AsJsonString(m_client.GetAccountId()).c_str(), e.what());

	SmtpCommand::Failure(e);
}

void SmtpAccountEnableCommand::Done()
{
	if(m_msg.get())
		m_msg->replySuccess();

	MojLogNotice(m_log, "successfully enabled SMTP on account %s", AsJsonString(m_client.GetAccountId()).c_str());

	Complete();
}
