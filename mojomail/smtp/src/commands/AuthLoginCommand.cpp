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

#include "commands/AuthLoginCommand.h"
#include <iostream>

const char* const AuthLoginCommand::COMMAND_STRING		= "AUTH LOGIN";

AuthLoginCommand::AuthLoginCommand(SmtpSession& session)
: SmtpProtocolCommand(session),
  m_state(0)
{

}

AuthLoginCommand::~AuthLoginCommand()
{

}

void AuthLoginCommand::RunImpl()
{
	MojLogInfo(m_log, "Sending AUTH LOGIN command");

	SendCommand(COMMAND_STRING);
}

MojErr AuthLoginCommand::HandleResponse(const std::string& line)
{
	MojLogInfo(m_log, "AUTH LOGIN command response");

	// Form draft-murchison-sasl-login-00 LOGIN SASL payload, a section at a time

	if (m_state == 0) {
		if (m_statusCode == 334) {
			std::string user = m_session.GetAccount()->GetUsername();

			string encodedUsername;

			if(!user.empty()) {
				gchar* encoded = g_base64_encode((const guchar*)user.c_str(), user.length());
				if(encoded != NULL) {
					encodedUsername.assign(encoded);
					g_free(encoded);
				} else {
					MojLogError(m_log, "error base64 encoding SMTP username");
				}
			} else {
				MojLogError(m_log, "no SMTP username available");
			}

			SendCommand(encodedUsername);
			m_state = 1;
		} else {
			SmtpSession::SmtpError error = GetStandardError();
			error.internalError = "AUTH LOGIN failed, no user prompt";
			m_session.AuthLoginFailure(error);
		}
	} else if (m_state == 1) {
		if (m_statusCode == 334) {
			std::string password = m_session.GetAccount()->GetPassword();

			string encodedPassword;

			if(!password.empty()) {
				gchar* encoded = g_base64_encode((const guchar*)password.c_str(), password.length());
				if(encoded != NULL) {
					encodedPassword.assign(encoded);
					g_free(encoded);
				} else {
					MojLogError(m_log, "error base64 encoding SMTP password");
				}
			} else {
				MojLogError(m_log, "no SMTP password available");
			}

			SendCommand(encodedPassword);
			m_state = 2;
		} else {
			SmtpSession::SmtpError error = GetStandardError();
			error.internalError = "AUTH LOGIN failed, no password prompt";
			m_session.AuthLoginFailure(error);
		}
	} else if (m_state == 2) {
		if (m_status == Status_Ok) {
			MojLogInfo(m_log, "AUTH LOGIN OK");
			
			m_session.AuthLoginSuccess();
		} else {
			SmtpSession::SmtpError error = GetStandardError();
			error.internalError = "AUTH LOGIN failed";
			m_session.AuthLoginFailure(error);
		}
		Complete();
	}

	return MojErrNone;
}
