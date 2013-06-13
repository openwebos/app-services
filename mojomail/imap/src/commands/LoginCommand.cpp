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

#include "commands/LoginCommand.h"
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "client/ImapSession.h"
#include "data/ImapAccount.h"
#include "ImapValidator.h"
#include <sstream>
#include "exceptions/ExceptionUtils.h"
#include "client/ImapRequestManager.h"
#include "ImapPrivate.h"

using namespace std;

const int LoginCommand::LOGIN_TIMEOUT = 30; // seconds

LoginCommand::LoginCommand(ImapSession& session)
: ImapSessionCommand(session),
  m_loginResponseSlot(this, &LoginCommand::LoginResponse)
{
}

LoginCommand::~LoginCommand()
{
}

void LoginCommand::RunImpl()
{
	CommandTraceFunction();

	try {
		// Invalidate capability list when logging in
		m_session.GetCapabilities().SetValid(false);

		string username = m_session.GetLoginSettings().GetUsername();
		string password = m_session.GetLoginSettings().GetPassword();

		if(username.empty() || password.empty()) {
			MojObject accountId = m_session.GetAccount()->GetAccountId();

			MojLogWarning(m_log, "no password set on account %s", AsJsonString(accountId).c_str());
			m_session.LoginFailure(MailError::BAD_USERNAME_OR_PASSWORD);
			Complete();
			return;
		}

		MojLogInfo(ImapRequestManager::s_protocolLog, "[session %p] sending LOGIN command", this);
		m_responseParser.reset(new CapabilityResponseParser(m_session, m_loginResponseSlot));

		stringstream ss;

		ss << "LOGIN " << QuoteString(username);
		ss << " " << QuoteString(password);

		// Send login command with logging disabled
		m_session.SendRequest(ss.str(), m_responseParser, LOGIN_TIMEOUT, false);
	} CATCH_AS_FAILURE
}

MojErr LoginCommand::LoginResponse()
{
	CommandTraceFunction();

	try {
		ImapStatusCode status = m_responseParser->GetStatus();
		string line = m_responseParser->GetResponseLine();

		if (status == OK) {
			// line reads "LOGIN Ok."
			m_session.LoginSuccess();
		} else if (status == NO) {
			// login failed
			MailError::ErrorCode errorCode = MailError::BAD_USERNAME_OR_PASSWORD;

			// Handle common error messages from Gmail
			if(boost::icontains(line, "Web login required")) {
				errorCode = MailError::ACCOUNT_WEB_LOGIN_REQUIRED;
			} else if(boost::icontains(line, "Too many simultaneous connections")) {
				errorCode = MailError::ACCOUNT_UNAVAILABLE;
			} else if(boost::icontains(line, "Your account is not enabled for IMAP use")) {
				errorCode = MailError::IMAP_NOT_ENABLED;
			} else if(boost::icontains(line, "System Error")) {
				errorCode = MailError::SERVER_ERROR;
			}

			m_session.LoginFailure(errorCode, line);
		} else if (status == BAD) {
			m_session.LoginFailure(MailError::ACCOUNT_UNKNOWN_AUTH_ERROR, line);
		} else {
			m_responseParser->CheckStatus();
		}

		Complete();
	} CATCH_AS_FAILURE
	
	return MojErrNone;
}

void LoginCommand::Failure(const std::exception& e)
{
	MailError::ErrorInfo errorInfo = ExceptionUtils::GetErrorInfo(e);

	if (m_session.IsValidator() && errorInfo.errorCode == MailError::CONNECTION_TIMED_OUT) {
		// Special error code if it times out during the login command during validation.
		// This is because MobileMe can take up to a minute to report a bad password, which is longer
		// than we're willing to wait for validation.
		errorInfo.errorCode = MailError::LOGIN_TIMEOUT;
	}

	m_session.LoginFailure(errorInfo.errorCode, errorInfo.errorText);
	ImapSessionCommand::Failure(e);
}
