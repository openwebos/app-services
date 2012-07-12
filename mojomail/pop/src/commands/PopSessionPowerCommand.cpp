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

#include "commands/PopSessionPowerCommand.h"

PopSessionPowerCommand::PopSessionPowerCommand(PopSession& session, const std::string& reason)
: PopSessionCommand(session),
  m_stayAwakeReason(reason),
  m_powerManager(session.GetPowerManager())
{
	PowerUp();
}

PopSessionPowerCommand::~PopSessionPowerCommand()
{

}

void PopSessionPowerCommand::PowerUp()
{
	m_powerUser.Start(m_powerManager, m_stayAwakeReason);
}

void PopSessionPowerCommand::PowerDone()
{
	m_powerUser.Stop();
}

void PopSessionPowerCommand::Cleanup()
{
	PowerDone();
}
