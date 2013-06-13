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

#include "commands/UpdateAccountErrorCommand.h"
#include "data/ImapAccountAdapter.h"
#include "data/EmailAccountAdapter.h"
#include "data/DatabaseInterface.h"
#include "client/ImapSession.h"
#include "ImapPrivate.h"

UpdateAccountErrorCommand::UpdateAccountErrorCommand(ImapClient& client, const MailError::ErrorInfo& error)
: ImapClientCommand(client),
  m_error(error),
  m_updateAccountErrorSlot(this, &UpdateAccountErrorCommand::UpdateAccountErrorResponse)
{
}

UpdateAccountErrorCommand::~UpdateAccountErrorCommand()
{
}

void UpdateAccountErrorCommand::RunImpl()
{
	UpdateAccountError();
}

void UpdateAccountErrorCommand::UpdateAccountError()
{
	CommandTraceFunction();

	MojErr err;
	MojObject errInfo;

	if(m_error.errorCode != MailError::NONE) {
		err = errInfo.putInt(EmailAccountAdapter::ERROR_CODE, m_error.errorCode);
		ErrorToException(err);

		err = errInfo.putString(EmailAccountAdapter::ERROR_TEXT, m_error.errorText.c_str());
		ErrorToException(err);

		MojLogNotice(m_log, "storing error %s on account %s", AsJsonString(errInfo).c_str(), m_client.GetAccountIdString().c_str());
	} else {
		// Clear error object
		errInfo = MojObject::Null;

		MojLogInfo(m_log, "clearing error on account %s", m_client.GetAccountIdString().c_str());
	}

	MojObject account;
	err = account.put(EmailAccountAdapter::ERROR, errInfo);
	ErrorToException(err);

	m_client.GetDatabaseInterface().UpdateAccountError(m_updateAccountErrorSlot, m_client.GetAccountId(), account);
}

MojErr UpdateAccountErrorCommand::UpdateAccountErrorResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}
