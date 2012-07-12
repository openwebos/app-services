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

#include "commands/AuthYahooCommand.h"
#include <boost/lexical_cast.hpp>
#include <iostream>
#include <time.h>

const char* const AuthYahooCommand::COMMAND_STRING		= "AUTH XYMCOOKIE";

AuthYahooCommand::AuthYahooCommand(SmtpSession& session)
: SmtpProtocolCommand(session),
  m_getYahooCookiesSlot(this, &AuthYahooCommand::GetYahooCookiesSlot)
{

}

AuthYahooCommand::~AuthYahooCommand()
{

}

unsigned long long AuthYahooCommand::timeMillis()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	return (tv.tv_sec * 1000ULL) + (tv.tv_usec / 1000ULL);
}

void AuthYahooCommand::RunImpl()
{
	MojLogTrace(m_log);
	// HACK: Hard-code partner ID and fetch of NDUID
	m_partnerId.assign("mblclt11");
	
	char id[256];
	memset(id, '\0', 256);
	
	// Read nduid, if available, otherwise make a fake one and try to record it.
	FILE * nduid = fopen("/proc/nduid", "r");
	if (!nduid) {
		nduid = fopen("yahooCachedFakeDeviceId", "r");
		if (!nduid) {
			snprintf(id, 255, "FEED0BEEF479121481533145%016llX", timeMillis());
			nduid = fopen("yahooCachedFakeDeviceId", "w");
			if (nduid) {
				fputs(id, nduid);
				fclose(nduid);
				nduid = NULL;
			}
		}
	}
	
	if (nduid) {
		if (fgets(id, 255, nduid) == NULL)
			id[0] = '\0';
		
		fclose(nduid);
		nduid = NULL;

		if (strlen(id) > 0 && id[strlen(id)-1] == '\n')
			id[strlen(id)-1] = '\0';
	}
	
	m_deviceId.assign(id);

	MojLogInfo(m_log, "Temp: partnerId=%s deviceId=%s", m_partnerId.data(), m_deviceId.data());

	MojObject accountId = m_session.GetAccount()->GetAccountId();

	MojObject params;
	MojErr err = params.put("accountId", accountId);
	ErrorToException(err);

	MojString token;
	token.assign(m_session.GetAccount()->GetYahooToken().c_str());
	err = params.put("securityToken", token);
	ErrorToException(err);

	MojLogInfo(m_log, "Looking up yahoo cookies for account %s", AsJsonString(accountId).c_str());
	// TODO: Should send the request with SmtpClient instead.
	err = m_session.CreateRequest()->send(m_getYahooCookiesSlot, "com.palm.yahoo","fetchcookies", params);
	ErrorToException(err);
}

MojErr AuthYahooCommand::GetYahooCookiesSlot(MojObject& response, MojErr err)
{
	MojLogTrace(m_log);
	try {
		ResponseToException(response, err);

		MojObject object;
		err = response.getRequired("tCookie", m_tCookie);
		ErrorToException(err);
	
		err = response.getRequired("yCookie", m_yCookie);
		ErrorToException(err);
	
	} catch (const std::exception& e) {
		MojLogInfo(m_log, "Failed to retrieve Yahoo cookies, failing login: %s", e.what());
		
		// FIXME: We should possibly try to decode and pass along a specific error
		
		SmtpSession::SmtpError error;
		error.errorCode = MailError::ACCOUNT_UNAVAILABLE;
		error.errorOnAccount = true;
		error.internalError = std::string("Unable to retrieve yahoo cookies: ") + e.what();
		m_session.AuthYahooFailure(error);
		Complete();
		
		return MojErrNone;
	} catch (...) {
		MojLogInfo(m_log, "Failed to retrieve Yahoo cookies, failing login: unknown exception");

		// FIXME: We should possibly try to decode and pass along a specific error

		SmtpSession::SmtpError error;
		error.errorCode = MailError::ACCOUNT_UNAVAILABLE;
		error.errorOnAccount = true;
		error.internalError = "Unable to retrieve yahoo cookies: unknown exception";
		m_session.AuthYahooFailure(error);
		Complete();

		return MojErrNone;
	}

	m_state = State_SendingYCookie;
	WriteCommand();
	
	return MojErrNone;
}

void AuthYahooCommand::WriteCommand()
{
	MojLogTrace(m_log);
	if (m_state == State_SendingYCookie) {
		MojLogInfo(m_log, "Sending AUTH XYMCOOKIE command with YCookie");
	
		std::string command = COMMAND_STRING;
	
		std::string cookie = m_yCookie.data();
		cookie.erase(0,2); // erase 'Y='

		command += ' ';

		MojLogInfo(m_log, "Pre-encoded command: {%s}", cookie.c_str());
		
		gchar * encodedPayload = g_base64_encode((unsigned guchar*)cookie.c_str(), cookie.length());
		try {
			command += encodedPayload;
		} catch (...) {
			g_free(encodedPayload);
			throw;
		}
		g_free(encodedPayload);


		MojLogInfo(m_log, "Post-encoded command: {%s}", command.c_str());
	
		SendCommand(command);
		
	} else if (m_state == State_SendingTCookie) {
		MojLogInfo(m_log, "Sending TCookie");
	
		std::string cookie = m_tCookie.data();
		cookie.erase(0,2); // erase 'T='
		
		cookie += " ts=";
		cookie += boost::lexical_cast<std::string>(timeMillis() / 1000);
		
		cookie += " src=";
		cookie += m_partnerId.data();
		cookie += " GUID=";
		cookie += m_deviceId.data();
		
		MojLogInfo(m_log, "Pre-encoded cookie: {%s}", cookie.c_str());

		gchar * encodedPayload = g_base64_encode((unsigned guchar*)cookie.c_str(), cookie.length());
		try {
			cookie = encodedPayload;
		} catch (...) {
			g_free(encodedPayload);
			throw;
		}
		g_free(encodedPayload);

		MojLogInfo(m_log, "Post-encoded cookie: {%s}", cookie.c_str());
		
		SendCommand(cookie);
	}
}

MojErr AuthYahooCommand::HandleResponse(const std::string& line)
{
	SmtpSession::SmtpError error;

	MojLogInfo(m_log, "AUTH XYMCOOKIE command response");

	if (m_state == State_SendingYCookie) {
		if (m_statusCode == 334) {
			m_state = State_SendingTCookie;
			WriteCommand();
		} else {
			error = GetStandardError();
			error.internalError = "AUTH XYMCOOKIE, no tcookie prompt";
			m_session.AuthYahooFailure(error);
		}
	} else if (m_state == State_SendingTCookie) {
		if (m_status == Status_Ok) {
			MojLogInfo(m_log, "AUTH XYMCOOKIE OK");
			
			m_session.AuthYahooSuccess();
		} else {
			error = GetStandardError();
			error.internalError = "AUTH XYMCOOKIE failed at ycookie prompt";
			m_session.AuthYahooFailure(error);
		}
		Complete();
	}

	return MojErrNone;
}
