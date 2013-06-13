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

#include "commands/UpdateAccountStatusCommand.h"
#include "data/PopAccountAdapter.h"

UpdateAccountStatusCommand::UpdateAccountStatusCommand(PopSession& session,
		boost::shared_ptr<PopAccount> account,
		MailError::ErrorCode errCode,
		const std::string& errMsg)
: PopSessionCommand(session),
  m_account(account),
  m_errorCode(errCode),
  m_errorMessage(errMsg),
  m_updateResponseSlot(this, &UpdateAccountStatusCommand::UpdateAccountResponse)
{
}

UpdateAccountStatusCommand::~UpdateAccountStatusCommand()
{
}

void UpdateAccountStatusCommand::RunImpl()
{
	UpdateAccount();
}

void UpdateAccountStatusCommand::UpdateAccount()
{
	// update account object with error code and status
	EmailAccount::AccountError accntErr;
	accntErr.errorCode = m_errorCode;
	accntErr.errorText = m_errorMessage;
	m_account->SetError(accntErr);
	m_session.SetError(m_errorCode, m_errorMessage);

	MojObject mojAccount;
	PopAccountAdapter::SerializeToDatabasePopObject(*m_account.get(), mojAccount);
	m_session.GetDatabaseInterface().UpdateItem(m_updateResponseSlot, mojAccount);
}

MojErr UpdateAccountStatusCommand::UpdateAccountResponse(MojObject& response, MojErr err)
{
	ErrorToException(err);

	Complete();
	return MojErrNone;
}
