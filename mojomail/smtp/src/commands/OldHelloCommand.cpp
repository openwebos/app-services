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

#include "commands/OldHelloCommand.h"

const char* const OldHelloCommand::COMMAND_STRING				= "HELO";

OldHelloCommand::OldHelloCommand(SmtpSession& session, const std::string & serverName)
: SmtpProtocolCommand(session),
  m_serverName(serverName)
{
}

OldHelloCommand::~OldHelloCommand()
{

}

void OldHelloCommand::RunImpl()
{
	SendCommand(std::string(COMMAND_STRING) + " " + m_serverName);
}

MojErr OldHelloCommand::HandleResponse(const std::string& line)
{

	if (m_statusCode == 250) {
		m_session.HasSizeExtension(false);
		m_session.HasTLSExtension(false);
		m_session.HasChunkingExtension(false);
		m_session.HasPipeliningExtension(false);
		m_session.HasAuthExtension(false);
		m_session.HasPlainAuth(false);
		m_session.HasLoginAuth(false);
		m_session.HasYahooAuth(false);
		m_status = Status_Ok;
		
		m_session.HelloSuccess();
	} else {
		m_status = Status_Err;

		SmtpSession::SmtpError error = GetStandardError();
		error.internalError = "HLO failed";
		m_session.HelloFailure(error);
	}
		
	Complete();

	return MojErrNone;
}
