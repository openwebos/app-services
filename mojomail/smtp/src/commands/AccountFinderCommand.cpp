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

#include "commands/AccountFinderCommand.h"
#include "data/SmtpAccountAdapter.h"

AccountFinderCommand::AccountFinderCommand(SmtpSession& session,
										   MojObject accountId)
: SmtpCommand(session),
  m_session(session),
  m_account(new SmtpAccount()),
  m_accountId(accountId),
  m_getAccountSlot(this, &AccountFinderCommand::GetAccountResponse),
  m_getCommonCredentialsResponseSlot(this, &AccountFinderCommand::GetCommonCredentialsResponse),
  m_getSmtpCredentialsResponseSlot(this, &AccountFinderCommand::GetSmtpCredentialsResponse),
  m_getSmtpAccountSlot(this, &AccountFinderCommand::GetSmtpAccountResponse)
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

void AccountFinderCommand::Cancel()
{

}

void AccountFinderCommand::GetAccount()
{
	CommandTraceFunction();

	try {
		MojObject params;
		MojErr err = params.put("accountId", m_accountId);
		ErrorToException(err);

		MojLogInfo(m_log, "Looking up account %s", AsJsonString(m_accountId).c_str());
		// TODO: Should send the request with SmtpClient instead.
		err = m_session.CreateRequest()->send(m_getAccountSlot, "com.palm.service.accounts","getAccountInfo", params);
		ErrorToException(err);
	} catch (const std::exception& e) {
		MojLogInfo(m_log, "Exception setting up account query: %s", e.what());
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = std::string("AccountFinderCommand::GetAcccount caught exception: ")+ e.what();
		m_session.AccountFinderFailure(error);
		Complete();
	} catch (...) {
		MojLogInfo(m_log, "Exception setting up account query: unknown exception");
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = "AccountFinderCommand::GetAcccount caught unknown exception";
		m_session.AccountFinderFailure(error);
		Complete();
	}

	
}

MojErr AccountFinderCommand::GetAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check the result first
		ResponseToException(response, err);

		err = response.getRequired("result", m_accountObj);
		ErrorToException(err);
		MojLogInfo(m_log, "Found account");

		// get the password
		GetCommonCredentials();

	} catch (const std::exception& e) {
		MojLogInfo(m_log, "Exception getting account response: %s", e.what());
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = std::string("AccountFinderCommand::GetAccountResponse caught exception: ") + e.what();
		m_session.AccountFinderFailure(error);
		Complete();
	} catch (...) {
		MojLogInfo(m_log, "Exception getting account response: unknown exception");
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = "AccountFinderCommand::GetAccountResponse caught unknown exception";
		m_session.AccountFinderFailure(error);
		Complete();
	}

	return MojErrNone;
}


void AccountFinderCommand::GetCommonCredentials()
{
	CommandTraceFunction();
	
	try {

		MojObject params;
		MojErr err = params.put("accountId", m_accountId);
		ErrorToException(err);

		err = params.putString("name", "common");
		ErrorToException(err);
		// TODO: Should send the request with SmtpClient instead.
		err = m_session.CreateRequest()->send(m_getCommonCredentialsResponseSlot, "com.palm.service.accounts", "readCredentials", params);
		ErrorToException(err);
	} catch (const std::exception& e) {
		MojLogInfo(m_log, "Exception setting up common credentials query: %s", e.what());
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = std::string("AccountFinderCommand::GetCommonCredentials caught exception: ") + e.what();
		m_session.AccountFinderFailure(error);
		Complete();
	} catch (...) {
		MojLogInfo(m_log, "Exception setting up common credentials query: unknown exception");
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = "AccountFinderCommand::GetCommonCredentials caught unknown exception";
		m_session.AccountFinderFailure(error);
		Complete();
	}
		
}

MojErr AccountFinderCommand::GetCommonCredentialsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check the result first
		ResponseToException(response, err);

		MojObject credentials;
		if (response.get("credentials", credentials)) {

			// Optional password
			bool hasPassword = false;
			MojString password;
			err = credentials.get("password", password, hasPassword);
			ErrorToException(err);
			if(hasPassword) {
				m_account->SetPassword(password.data());
			}

			// Optional security token for Yahoo
			bool hasSecurityToken = false;
			MojString securityToken;
			err = credentials.get("securityToken", securityToken, hasSecurityToken);
			ErrorToException(err);
			if(hasSecurityToken) {
				m_account->SetYahooToken(securityToken.data());
			}
		}
		
		// get the SMTP credentials if there is any.
		GetSmtpCredentials();

	} catch (const std::exception& e) {
		MojLogInfo(m_log, "Exception getting password response: %s", e.what());
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = std::string("AccountFinderCommand::GetCommonCredentialsResponse caught exception: ") + e.what();
		m_session.AccountFinderFailure(error);
		Complete();
	} catch (...) {
		MojLogInfo(m_log, "Exception getting password response: unknown exception");
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = "AccountFinderCommand::GetCommonCredentialsResponse caught unknown exception";
		m_session.AccountFinderFailure(error);
		Complete();
	}

	return MojErrNone;
}

void AccountFinderCommand::GetSmtpCredentials()
{
	CommandTraceFunction();
	
	try {

		MojObject params;
		MojErr err = params.put("accountId", m_accountId);
		ErrorToException(err);

		err = params.putString("name", "common");
		ErrorToException(err);
		// TODO: Should send the request with SmtpClient instead.
		err = m_session.CreateRequest()->send(m_getSmtpCredentialsResponseSlot, "com.palm.service.accounts", "readCredentials", params);
		ErrorToException(err);
	} catch (const std::exception& e) {
		MojLogInfo(m_log, "Exception setting up smtp credentials query: %s", e.what());
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = std::string("AccountFinderCommand::GetSmtpCredentials caught exception: ") + e.what();
		m_session.AccountFinderFailure(error);
		Complete();
	} catch (...) {
		MojLogInfo(m_log, "Exception setting up smtp credentials query: unknown exception");
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = "AccountFinderCommand::GetSmtpCredentials caught unknown exception";
		m_session.AccountFinderFailure(error);
		Complete();
	}


}

MojErr AccountFinderCommand::GetSmtpCredentialsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	
	// Fetch Smtp credentials after common credentials: any password or token in the
	// smtp credentials should win over the common credentials.

	try {
		// check the result first
		ResponseToException(response, err);

		MojObject credentials;
		if (response.get("credentials", credentials)) {

			// Optional password
			bool hasPassword = false;
			MojString password;
			err = credentials.get("password", password, hasPassword);
			ErrorToException(err);
			if(hasPassword) {
				m_account->SetPassword(password.data());
			}

			// Optional security token for Yahoo
			bool hasSecurityToken = false;
			MojString securityToken;
			err = credentials.get("securityToken", securityToken, hasSecurityToken);
			ErrorToException(err);
			if(hasSecurityToken) {
				m_account->SetYahooToken(securityToken.data());
			}
		}

		// get the transport object
		GetSmtpAccount();

	} catch (const std::exception& e) {
		MojLogInfo(m_log, "Exception getting password response: %s", e.what());
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = std::string("AccountFinderCommand::GetSmtpCredentialsResponse caught exception: ") + e.what();
		m_session.AccountFinderFailure(error);
		Complete();
	} catch (...) {
		MojLogInfo(m_log, "Exception getting password response: unknown exception");
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = "AccountFinderCommand::GetSmtpCredentialsResponse caught unknown exception";
		m_session.AccountFinderFailure(error);
		Complete();
	}

	return MojErrNone;
}

void AccountFinderCommand::GetSmtpAccount()
{
	CommandTraceFunction();
	try {
		m_session.GetDatabaseInterface().GetAccount(m_getSmtpAccountSlot, m_accountId);
	} catch (const std::exception& e) {
		MojLogInfo(m_log, "Exception setting up smtp account query: %s", e.what());
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = std::string("AccountFinderCommand::GetSmtpAccount caught exception: ") + e.what();
		m_session.AccountFinderFailure(error);
		Complete();
	} catch (...) {
		MojLogInfo(m_log, "Exception setting up smtp account query: unknown exception");
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = "AccountFinderCommand::GetSmtpAccount caught unknown exception";
		m_session.AccountFinderFailure(error);
		Complete();
	}
}

MojErr AccountFinderCommand::GetSmtpAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {

		MojLogInfo(m_log, "Got smtp account response");
		MojLogDebug(m_log, "The response is %s", AsJsonString(response).c_str());

		// check database result first
		ResponseToException(response, err);

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);
		MojLogDebug(m_log, "result: %s", AsJsonString(results).c_str());

		if (results.size() > 0) {
			MojObject transportObj;
			if (results.at(0, transportObj)) {
				MojLogDebug(m_log, "transportObj: %s", AsJsonString(transportObj).c_str());
				MojLogDebug(m_log, "m_accountObj: %s", AsJsonString(m_accountObj).c_str());
				SmtpAccountAdapter::GetSmtpAccount(m_accountObj, transportObj, m_account);
			}
		}
		
		MojLogInfo(m_log, "Setting account to smtp client");
		m_session.AccountFinderSuccess(m_account);
		Complete();
	} catch (const std::exception& e) {
		MojLogInfo(m_log, "found exception: %s", e.what());
		// Don't set account, session remains in NeedsAccount state
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = std::string("AccountFinderCommand::GetSmtpAccountResponse caught exception: ") + e.what();
		m_session.AccountFinderFailure(error);
		Complete();
	} catch (...) {
		MojLogInfo(m_log, "found exception: unknown exception");
		// Don't set account, session remains in NeedsAccount state
		SmtpSession::SmtpError error;
		error.errorCode = MailError::INTERNAL_ACCOUNT_MISCONFIGURED;
		error.errorOnAccount = true;
		error.internalError = "AccountFinderCommand::GetSmtpAccountResponse caught unknown exception";
		m_session.AccountFinderFailure(error);
		Complete();
	}

	return MojErrNone;
}

