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

#include "commands/DeleteAccountCommand.h"
#include "ImapClient.h"
#include "data/DatabaseInterface.h"
#include "ImapPrivate.h"
#include "core/MojServiceMessage.h"

DeleteAccountCommand::DeleteAccountCommand(ImapClient& client, const MojRefCountedPtr<MojServiceMessage>& msg, const MojObject& payload)
: ImapClientCommand(client),
  m_msg(msg),
  m_payload(payload),
  m_isYahoo(false),
  m_getAccountSlot(this, &DeleteAccountCommand::GetAccountInfoResponse),
  m_deleteAccountSlot(this, &DeleteAccountCommand::DeleteAccountResponse),
  m_smtpAccountDeletedSlot(this, &DeleteAccountCommand::SmtpAccountDeletedResponse),
  m_yahooAccountDeletedSlot(this, &DeleteAccountCommand::YahooAccountDeletedResponse)
{
}

DeleteAccountCommand::~DeleteAccountCommand()
{
}

void DeleteAccountCommand::RunImpl()
{
	GetAccountInfo();
}

void DeleteAccountCommand::GetAccountInfo()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "getting account info to delete account");

	MojErr err;
	MojObject params;

	err = params.put("accountId", m_client.GetAccountId());
	ErrorToException(err);

	m_client.SendRequest(m_getAccountSlot, "com.palm.service.accounts","getAccountInfo", params);
}

MojErr DeleteAccountCommand::GetAccountInfoResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		MojObject result;
		err = response.getRequired("result", result);
		ErrorToException(err);

		MojString templateId;
		bool hasTemplateId = false;
		err = result.get(ImapAccountAdapter::TEMPLATEID, templateId, hasTemplateId);
		ErrorToException(err);

		if(hasTemplateId) {
			if(templateId == "com.palm.yahoo") { // FIXME use constant
				m_isYahoo = true;
			}
		}

		DeleteAccount();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DeleteAccountCommand::DeleteAccount()
{
	CommandTraceFunction();

	m_client.GetDatabaseInterface().DeleteAccount(m_deleteAccountSlot, m_client.GetAccountId());
}

MojErr DeleteAccountCommand::DeleteAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		// Tell SMTP service to clean up any SMTP state
		DeleteSmtpAccount();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

// TODO: Should do this *before* deleting the account
void DeleteAccountCommand::DeleteSmtpAccount()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "notifying SMTP service of account deletion");

	m_client.SendRequest(m_smtpAccountDeletedSlot, "com.palm.smtp", "accountDeleted", m_payload);
}

MojErr DeleteAccountCommand::SmtpAccountDeletedResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		if(m_isYahoo) {
			DeleteYahooAccount();
		} else {
			Done();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DeleteAccountCommand::DeleteYahooAccount()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "notifying Yahoo service of account deletion");

	m_client.SendRequest(m_yahooAccountDeletedSlot, "com.palm.yahoo", "accountdeleted", m_payload);
}

MojErr DeleteAccountCommand::YahooAccountDeletedResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		if(err) {
			try {
				ResponseToException(response, err);
			} catch(const std::exception& e) {
				// Ignore error
				MojLogWarning(m_log, "error cleaning up Yahoo service data: %s", e.what());
			}
		}

		Done();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DeleteAccountCommand::Done()
{
	CommandTraceFunction();

	m_client.DeletedAccount();

	if(m_msg.get()) {
		m_msg->replySuccess();
	}

	Complete();
}
