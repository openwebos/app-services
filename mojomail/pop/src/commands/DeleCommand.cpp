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

#include "commands/DeleCommand.h"

const char* const DeleCommand::COMMAND_STRING		= "DELE";

DeleCommand::DeleCommand(PopSession& session, int msgNum)
: PopProtocolCommand(session),
  m_msgNum(msgNum)
{
}

DeleCommand::~DeleCommand()
{
}

void DeleCommand::RunImpl()
{
	// Command syntax: "DELE <msg_num>".
	std::ostringstream command;
	command << COMMAND_STRING << " " << m_msgNum;

	SendCommand(command.str());
}

MojErr DeleCommand::HandleResponse(const std::string& line)
{
	Complete();

	return MojErrNone;
}
