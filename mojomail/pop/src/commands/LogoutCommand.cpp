// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

const char* const LogoutCommand::COMMAND_STRING		= "QUIT";

LogoutCommand::LogoutCommand(PopSession& session)
: PopProtocolCommand(session)
{

}

LogoutCommand::~LogoutCommand()
{

}

void LogoutCommand::RunImpl()
{
	SendCommand(COMMAND_STRING);
}

MojErr LogoutCommand::HandleResponse(const std::string& line)
{
	if (m_status != Status_Ok) {
		MojLogError(m_log, "Error response in logging out pop session: %s", line.c_str());
	}

	m_session.LogoutDone();
	Complete();
	return MojErrNone;
}

void LogoutCommand::Failure(const std::exception& ex)
{
	m_session.LogoutDone();
	PopSessionCommand::Failure(ex);
}
