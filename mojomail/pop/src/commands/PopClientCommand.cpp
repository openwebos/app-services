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

#include "commands/PopClientCommand.h"

using namespace std;

PopClientCommand::PopClientCommand(PopClient& client, const std::string& reason, Priority priority)
: PopCommand(client, priority),
  m_client(client),
  m_stayAwakeReason(reason),
  m_powerManager(client.GetPowerManager())
{
	PowerUp();
}

PopClientCommand::~PopClientCommand()
{

}

void PopClientCommand::Complete()
{
	// clean up command's resource and them completes the command
	Cleanup();
	Command::Complete();

	MojRefCountedPtr<PopCommandResult> result = GetResult();
	result->Done();
}

void PopClientCommand::Failure(const exception& exc)
{
	MojLogError(m_log, "Exception in command %s: %s", Describe().c_str(), exc.what());
	Cleanup();
	m_client.CommandFailed(this, MailError::INTERNAL_ERROR, exc);

	MojRefCountedPtr<PopCommandResult> result = GetResult();
	result->SetException(exc);
	result->Done();
}

void PopClientCommand::PowerUp()
{
	m_powerUser.Start(m_powerManager, m_stayAwakeReason);
}

void PopClientCommand::PowerDone()
{
	m_powerUser.Stop();
}

void PopClientCommand::Cleanup()
{
	PowerDone();
}
