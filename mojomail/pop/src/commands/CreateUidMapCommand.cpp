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

#include "commands/CreateUidMapCommand.h"

CreateUidMapCommand::CreateUidMapCommand(PopSession& session, boost::shared_ptr<UidMap>& uidMap)
: PopSessionCommand(session),
  m_uidMapPtr(uidMap),
  m_getUidlCommandResponseSlot(this, &CreateUidMapCommand::GetUidlCommandResponse),
  m_getListCommandResponseSlot(this, &CreateUidMapCommand::GetListCommandResponse)
{

}
CreateUidMapCommand::~CreateUidMapCommand()
{

}

void CreateUidMapCommand::RunImpl()
{
	m_commandResult.reset(new PopCommandResult(m_getUidlCommandResponseSlot));

	m_uidlCommand.reset(new UidlCommand(m_session, m_uidMapPtr));
	m_uidlCommand->SetResult(m_commandResult);
	m_uidlCommand->Run();
}

MojErr CreateUidMapCommand::GetUidlCommandResponse()
{
	try {
		m_commandResult->CheckException();

		m_commandResult.reset(new PopCommandResult(m_getListCommandResponseSlot));
		m_listCommand.reset(new ListCommand(m_session, m_uidMapPtr));
		m_listCommand->SetResult(m_commandResult);
		m_listCommand->Run();
	} catch (const std::exception& ex) {
		Failure(ex);
	}

	return MojErrNone;
}

MojErr CreateUidMapCommand::GetListCommandResponse()
{
	m_session.GotUidMap();
	Complete();

	return MojErrNone;
}
