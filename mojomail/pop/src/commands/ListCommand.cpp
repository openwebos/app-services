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

#include "commands/ListCommand.h"

const char* const ListCommand::COMMAND_STRING		= "LIST";

ListCommand::ListCommand(PopSession& session, boost::shared_ptr<UidMap>& uidMapPtr)
: PopMultiLineResponseCommand(session),
  m_uidMapPtr(uidMapPtr)

{

}

ListCommand::~ListCommand()
{

}

void ListCommand::RunImpl()
{
	CommandTraceFunction();
	std::string command = COMMAND_STRING;
	SendCommand(command);
}

MojErr ListCommand::HandleResponse(const std::string& line)
{
	// line format: 'msgNum uid'
	size_t ndx = line.find(' ');
	std::string numStr = line.substr(0, ndx);
	int msgNum = atoi(numStr.c_str());
	std::string sizeStr = line.substr(ndx + 1, line.length());
	int size = atoi(sizeStr.c_str());

	MojLogDebug(m_log, "Setting message size %i to message info for message %i into UID map", size, msgNum);
	m_uidMapPtr->SetMessageSize(msgNum, size);
	return MojErrNone;
}
