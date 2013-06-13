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

#include "commands/AccountFinderCommand.h"
#include "data/PopAccountAdapter.h"

AccountFinderCommand::AccountFinderCommand(PopClient& client, MojObject accountId)
: PopClientCommand(client, "Load POP account", HighPriority),
  m_account(new PopAccount()),
  m_accountId(accountId),
  m_getAccountSlot(this, &AccountFinderCommand::GetAccountResponse),
  m_getPasswordSlot(this, &AccountFinderCommand::GetPasswordResponse),
  m_getPopAccountSlot(this, &AccountFinderCommand::GetPopAccountResponse)
{
	m_account->SetAccountId(accountId);
}

AccountFinderCommand::~AccountFinderCommand()
{

}

void AccountFinderCommand::RunImpl()
{
	CommandTraceFunction();

	GetAccount();
}

void AccountFinderCommand::GetAccount()
{
	CommandTraceFunction();

	try {
		MojObject params;
		MojErr err = params.put("accountId", m_accountId);
		ErrorToException(err);

		err = m_client.CreateRequest()->send(m_getAccountSlot, "com.palm.service.accounts","getAccountInfo", params);
		ErrorToException(err);
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}
}

MojErr AccountFinderCommand::GetAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check the result first
		ErrorToException(err);

		err = response.getRequired("result", m_accountObj);
		ErrorToException(err);

		MojLogInfo(m_log, "Found account %s", AsJsonString(m_accountId).c_str());

		// get the password
		GetPassword();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void AccountFinderCommand::GetPassword()
{
	CommandTraceFunction();

	try {
		MojObject params;
		MojErr err = params.put("accountId", m_accountId);
		ErrorToException(err);

		err = params.putString("name", "common");
		ErrorToException(err);

		err = m_client.CreateRequest()->send(m_getPasswordSlot, "com.palm.service.accounts","readCredentials", params);
		ErrorToException(err);
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}
}

MojErr AccountFinderCommand::GetPasswordResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check the result first
		ErrorToException(err);

		MojObject credentials;
		// Credentials object can be missing if the POP account is restored from
		// backup.  Checking for its existence before using it will prevent exception
		// to be thrown.
		bool exists = response.get("credentials", credentials);
		if (exists) {
			MojString password;
			err = credentials.getRequired("password", password);
			ErrorToException(err);

			m_account->SetPassword(password.data());
		} else {
			MojLogInfo(m_log, "The credentials of POP account '%s' are missing", AsJsonString(m_accountId).c_str());
		}

		// get the transport object
		GetPopAccount();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void AccountFinderCommand::GetPopAccount()
{
	m_client.GetDatabaseInterface().GetAccount(m_getPopAccountSlot, m_accountId);
}

MojErr AccountFinderCommand::GetPopAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check database result first
		ErrorToException(err);

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);

		if (results.size() > 0) {
			MojObject transportObj;
			if (results.at(0, transportObj)) {
				PopAccountAdapter::GetPopAccount(m_accountObj, transportObj, *(m_account.get()));
			}
		}

		MojLogInfo(m_log, "Setting account to pop client");
		m_client.SetAccount(m_account);

		Complete();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void AccountFinderCommand::Failure(const std::exception& exc)
{
	m_client.FindAccountFailed(m_accountId);
	PopClientCommand::Failure(exc);
}
