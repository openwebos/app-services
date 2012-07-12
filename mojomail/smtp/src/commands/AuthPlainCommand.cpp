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

#include "commands/AuthPlainCommand.h"
#include <iostream>

const char* const AuthPlainCommand::COMMAND_STRING		= "AUTH PLAIN";

AuthPlainCommand::AuthPlainCommand(SmtpSession& session)
: SmtpProtocolCommand(session)
{

}

AuthPlainCommand::~AuthPlainCommand()
{

}

void AuthPlainCommand::RunImpl()
{
	MojLogInfo(m_log, "Sending AUTH PLAIN command");
	
	std::string user = m_session.GetAccount()->GetUsername();
	std::string password = m_session.GetAccount()->GetPassword();

	// Form RFC-4616 PLAIN SASL message
	
	// Use std::string as char sequence
	std::string payload;
	
	payload.append(1, '\0');
	payload.append(user);
	payload.append(1, '\0');
	payload.append(password);
	
	gchar * encodedPayload = g_base64_encode((unsigned guchar*)payload.data(), payload.length());

	std::string userCommand = std::string(COMMAND_STRING) + " " + encodedPayload;
	SendCommand(userCommand);
}

MojErr AuthPlainCommand::HandleResponse(const std::string& line)
{
	MojLogInfo(m_log, "AUTH PLAIN command response");

	if (m_status == Status_Ok) {
		MojLogInfo(m_log, "AUTH PLAIN OK");

		m_session.AuthPlainSuccess();
	} else {
		SmtpSession::SmtpError error = GetStandardError();
		error.internalError = "AUTH PLAIN failed";
		m_session.AuthPlainFailure(error);
	}

	Complete();
	return MojErrNone;
}
