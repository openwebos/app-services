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

#include "commands/LogoutCommand.h"
#include "protocol/ImapResponseParser.h"
#include "client/ImapSession.h"

const int LogoutCommand::LOGOUT_RESPONSE_TIMEOUT = 5; // 5 seconds

LogoutCommand::LogoutCommand(ImapSession& session)
: ImapSessionCommand(session),
  m_logoutSlot(this, &LogoutCommand::LogoutResponse)
{
}

LogoutCommand::~LogoutCommand()
{
}

void LogoutCommand::RunImpl()
{
	SendLogout();
}

void LogoutCommand::SendLogout()
{
	try {
		m_responseParser.reset(new ImapResponseParser(m_session, m_logoutSlot));

		m_session.SendRequest("LOGOUT", m_responseParser, LOGOUT_RESPONSE_TIMEOUT);
	} CATCH_AS_FAILURE
}

MojErr LogoutCommand::LogoutResponse()
{
	try {
		m_session.LoggedOut();

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void LogoutCommand::Failure(const std::exception& e)
{
	ImapSessionCommand::Failure(e);

	m_session.LoggedOut();
}
