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

#include "commands/PasswordCommand.h"
#include "client/PopSession.h"
#include <iostream>

const char* const PasswordCommand::COMMAND_STRING		= "PASS";

PasswordCommand::PasswordCommand(PopSession& session, std::string pwd)
: PopProtocolCommand(session),
  m_password(pwd)
{

}

PasswordCommand::~PasswordCommand()
{

}

void PasswordCommand::RunImpl()
{
	if (!m_session.HasNetworkError()) {
		MojLogInfo(m_log, "Sending PASS command");

		std::string passwordCommand = COMMAND_STRING + (" " + m_password);
		SendCommand(passwordCommand);
	} else {
		MojLogInfo(m_log, "Abort password command due to network or login error");
		Complete();
	}
}

MojErr PasswordCommand::HandleResponse(const std::string& line)
{
	if (m_status == Status_Ok) {
		m_session.LoginSuccess();
	} else if(m_errorCode != MailError::NONE) {
		// Already have an error code from the error analyzer
		m_session.LoginFailure(m_errorCode, m_serverMessage);
	} else {
		m_session.LoginFailure(MailError::BAD_USERNAME_OR_PASSWORD, m_serverMessage);
	}

	Complete();
	return MojErrNone;
}
