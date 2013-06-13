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

#include "boost/shared_ptr.hpp"
#include "commands/UidlCommand.h"

const char* const UidlCommand::COMMAND_STRING		= "UIDL";

UidlCommand::UidlCommand(PopSession& session, boost::shared_ptr<UidMap>& uidMapPtr)
: PopMultiLineResponseCommand(session),
  m_uidMapPtr(uidMapPtr)

{
}

UidlCommand::~UidlCommand()
{

}

void UidlCommand::RunImpl()
{
	MojLogInfo(m_log, "Sending UIDL command");

	std::string command = COMMAND_STRING;
	SendCommand(command);
}

MojErr UidlCommand::HandleResponse(const std::string& line)
{
	// line format: 'msgNum uid'
	size_t ndx = line.find(' ');
	std::string numStr = line.substr(0, ndx);
	int msgNum = atoi(numStr.c_str());
	std::string uid = line.substr(ndx + 1, line.length());

	MojLogDebug(m_log, "Adding message number %i and uid %s into UID map", msgNum, uid.c_str());
	m_uidMapPtr->AddToMap(uid, msgNum);
	return MojErrNone;
}
