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

#include "commands/AuthYahooCommand.h"
#include "client/ImapSession.h"
#include "ImapPrivate.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <time.h>
#include "exceptions/ExceptionUtils.h"

const char* const AuthYahooCommand::AUTH_COMMAND_STRING	= "AUTHENTICATE XYMCOOKIEB64";
const char* const AuthYahooCommand::ID_COMMAND_STRING	= "ID";

const int AuthYahooCommand::FETCH_COOKIES_TIMEOUT = 120;

const int YAHOO_SERVICE_CONNECTION_ERROR = 2;
const int YAHOO_SERVICE_LOGIN_ERROR = 3;
const int YAHOO_SERVICE_CREDENTIALS_ERROR = 4;

AuthYahooCommand::AuthYahooCommand(ImapSession& session)
: ImapSessionCommand(session),
  m_loginFailureCount(0),
  m_getYahooCookiesSlot(this, &AuthYahooCommand::GetYahooCookiesResponse),
  m_authYahooDoResponseSlot(this, &AuthYahooCommand::HandleIDResponse),
  m_authYahooAuthenticateContinueSlot(this, &AuthYahooCommand::HandleAuthenticateContinue),
  m_authYahooAuthenticateDoneSlot(this, &AuthYahooCommand::HandleAuthenticateDone)
{

}

AuthYahooCommand::~AuthYahooCommand()
{

}

unsigned long long AuthYahooCommand::timeSec()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec;
}

void AuthYahooCommand::RunImpl()
{
	CommandTraceFunction();

	GetNduid();
}

void AuthYahooCommand::GetNduid()
{
	// FIXME get nduid and partnerId from Yahoo service
	MojLogDebug(m_log, "Getting nduid...");

	// FIXME HACK: Hard-code partner ID and fetch of NDUID
	m_partnerId.assign("mblclt11");

	char id[256];
	memset(id, '\0', 256);

	// Read nduid, if available, otherwise make a fake one and try to record it.
	FILE * nduid = fopen("/proc/nduid", "r");
	if (!nduid) {
		nduid = fopen("yahooCachedFakeDeviceId", "r");
		if (!nduid) {
			snprintf(id, 255, "FEED0BEEF479121481533145%016llX", timeSec());
			nduid = fopen("yahooCachedFakeDeviceId", "w");
			if (nduid) {
				fputs(id, nduid);
				fclose(nduid);
				nduid = NULL;
			}
		}
	}

	if (nduid) {
		char* ret = fgets(id, 255, nduid);

		fclose(nduid);
		nduid = NULL;

		if(ret) {

			if (strlen(id) > 0 && id[strlen(id)-1] == '\n')
				id[strlen(id)-1] = '\0';
		}
	}

	m_deviceId.assign(id);

	//MojLogInfo(m_log, "Temp: partnerId=%s deviceId=%s", m_partnerId.data(), m_deviceId.data());

	GetYahooCookies();
}

void AuthYahooCommand::GetYahooCookies()
{
	CommandTraceFunction();

	MojObject accountId = m_session.GetAccount()->GetAccountId();

	MojObject params;
	MojErr err = params.put("accountId", accountId);
	ErrorToException(err);

	const std::string& token = m_session.GetLoginSettings().GetYahooToken();

	if(!token.empty()) {
		err = params.putString("securityToken", token.c_str());
		ErrorToException(err);
	} else {
		// Missing security token (probably missing password)
		MojLogWarning(m_log, "no securityToken (missing password)");

		m_session.AuthYahooFailure(MailError::BAD_USERNAME_OR_PASSWORD, "");
		Complete();

		return;
	}

	// If this is a retry, send the old cookies to indicate that they're invalid
	if(m_loginFailureCount > 0) {
		err = params.put("tCookie", m_tCookie);
		ErrorToException(err);

		err = params.put("yCookie", m_yCookie);
		ErrorToException(err);

		err = params.put("crumb", m_crumb);
		ErrorToException(err);
	}

	MojLogInfo(m_log, "looking up yahoo cookies for account %s", AsJsonString(accountId).c_str());

	m_getCookiesTimer.SetTimeout(FETCH_COOKIES_TIMEOUT, this, &AuthYahooCommand::GetYahooCookiesTimeout);

	m_session.GetClient()->SendRequest(m_getYahooCookiesSlot, "com.palm.yahoo", "fetchcookies", params);
}

void AuthYahooCommand::GetYahooCookiesTimeout()
{
	CommandTraceFunction();

	MojLogCritical(m_log, "timed out waiting for cookies response from Yahoo service");

	MailException exc("timed out waiting for cookies", __FILE__, __LINE__);

	Failure(exc);
}

MojErr AuthYahooCommand::GetYahooCookiesResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	
	m_getCookiesTimer.Cancel();

	MojLogInfo(m_log, "got Yahoo cookies response from Yahoo service");
	try {
		if (err == YAHOO_SERVICE_LOGIN_ERROR) {
			MojLogError(m_log, "Yahoo service reported error requesting cookies from server");

			// FIXME: not necessarily bad password, could be a CAPTCHA error or something
			// Need Yahoo service to give a more detailed error code

			m_session.AuthYahooFailure(MailError::BAD_USERNAME_OR_PASSWORD, "");
			Complete();

			return MojErrNone;
		} else if(err == YAHOO_SERVICE_CONNECTION_ERROR) {
			MojLogError(m_log, "Yahoo service reported error connecting to auth server");

			m_session.AuthYahooFailure(MailError::CONNECTION_FAILED, "");
			Complete();

			return MojErrNone;
		} else {
			ResponseToException(response, err);

			MojObject object;
			MojErr err = response.getRequired("tCookie", m_tCookie);
			ErrorToException(err);

			err = response.getRequired("yCookie", m_yCookie);
			ErrorToException(err);

			err = response.getRequired("crumb", m_crumb);
			ErrorToException(err);
		}

	} catch (std::exception& e) {
		MojLogInfo(m_log, "failed to retrieve Yahoo cookies, failing login: exception: %s", e.what());
		m_session.AuthYahooFailure(MailError::ACCOUNT_UNKNOWN_AUTH_ERROR, e.what());
		Complete();

		return MojErrNone;
	}

	// Separate out sending ID request, since this could fail if the connection dies
	try {
		SendIDRequest();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void AuthYahooCommand::SendIDRequest()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "Sending ID command");

	stringstream idCommand;
	idCommand << ID_COMMAND_STRING << " (\"vendor\" \"Palm\" \"os\" \"Palm Nova\" \"os-version\" \"0.8\" \"GUID\" \""<< m_deviceId <<"\" \"device\" \"Palm Pre\")";
	m_authYahooResponseParser.reset(new ImapResponseParser(m_session, m_authYahooDoResponseSlot));
	m_session.SendRequest(idCommand.str(), m_authYahooResponseParser);
}

MojErr AuthYahooCommand::HandleIDResponse()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "IMAP ID command Response");
	try {
		ImapStatusCode status = m_authYahooResponseParser->GetStatus();
		string line = m_authYahooResponseParser->GetResponseLine();

		if (status == OK) {
			// line reads "ID Ok."
			SendAuthenticateRequest();
		} else if (status == NO) {
			// ID command failed
			m_session.AuthYahooFailure(MailError::ACCOUNT_UNAVAILABLE, line);
			Complete();
		} else {
			// This should throw an exception
			m_authYahooResponseParser->CheckStatus();
		}

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void AuthYahooCommand::SendAuthenticateRequest()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "Sending AUTHENTICATE command");

	m_authYahooResponseParser.reset(new ImapResponseParser(m_session, m_authYahooAuthenticateDoneSlot));
	m_authYahooResponseParser->SetContinuationResponseSlot(m_authYahooAuthenticateContinueSlot);
	m_session.SendRequest(AUTH_COMMAND_STRING, m_authYahooResponseParser);
}

MojErr AuthYahooCommand::HandleAuthenticateContinue()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "YAHOO AUTHENTICATE command Response + ");

	try {
		SendCookiesCommand();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void AuthYahooCommand::SendCookiesCommand()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "Sending cookies");

	stringstream cookies;

	cookies << m_yCookie.data() << " " << m_tCookie.data() << " ts=" << time(NULL) << " src=" << m_partnerId.data() << " cid=" << m_deviceId.data();

	MojLogDebug(m_log, "Pre-encoded cookies: {%s}", cookies.str().c_str());

	stringstream encodedCookies;

	gchar* encodedPayload = g_base64_encode((const guchar*) cookies.str().c_str(), cookies.str().length());
	encodedCookies << encodedPayload << "\r\n";
	g_free(encodedPayload);

	MojLogDebug(m_log, "Post-encoded cookies: {%s}", encodedCookies.str().c_str());

	OutputStreamPtr outPtr = m_session.GetOutputStream();
	outPtr->Write(encodedCookies.str());
	outPtr->Flush();
}

MojErr AuthYahooCommand::HandleAuthenticateDone()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "YAHOO AUTHENTICATE command completed");
	try {
		ImapStatusCode status = m_authYahooResponseParser->GetStatus();
		string line = m_authYahooResponseParser->GetResponseLine();

		if (status == OK) {
			// line reads "Authentication Ok."
			m_session.AuthYahooSuccess();
		} else if (status == NO) {
			// Authentication failed
			MailError::ErrorCode errorCode = MailError::BAD_USERNAME_OR_PASSWORD;

			if(boost::istarts_with(line, "[UNAVAILABLE]") || boost::istarts_with(line, "[INUSE]")) {
				errorCode = MailError::ACCOUNT_UNAVAILABLE;
			} else {
				// FIXME detect specific error for bad cookies
				if(m_loginFailureCount < 1) {
					m_loginFailureCount++;

					MojLogInfo(m_log, "retrying Yahoo login due to NO response: %s", line.c_str());

					// Retry
					GetYahooCookies();
					return MojErrNone;
				}
			}

			m_session.AuthYahooFailure(errorCode, line);
		} else {
			if(status == BAD) {
				// FIXME detect specific error for bad cookies
				if(m_loginFailureCount < 1) {
					m_loginFailureCount++;

					MojLogInfo(m_log, "retrying Yahoo login due to BAD response: %s", line.c_str());

					// Retry
					GetYahooCookies();
					return MojErrNone;
				}
			}

			// This should throw an exception
			m_authYahooResponseParser->CheckStatus();
		}

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void AuthYahooCommand::Failure(const exception& e)
{
	MailError::ErrorInfo errorInfo = ExceptionUtils::GetErrorInfo(e);

	m_session.AuthYahooFailure(errorInfo.errorCode, errorInfo.errorText);
	ImapSessionCommand::Failure(e);
}

void AuthYahooCommand::Cleanup()
{
	m_getYahooCookiesSlot.cancel();
	m_getCookiesTimer.Cancel();
}
