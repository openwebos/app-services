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

#ifndef POPCLIENTCOMMAND_H_
#define POPCLIENTCOMMAND_H_

#include "client/PowerManager.h"
#include "client/PowerUser.h"
#include "commands/PopCommand.h"
#include "PopClient.h"

class PopClient;

/**
 * All client commands will be power aware commands.
 */
class PopClientCommand : public PopCommand
{
public:
	PopClientCommand(PopClient& client, const std::string& reason, Command::Priority priority = NormalPriority);
	virtual ~PopClientCommand();

protected:
	virtual void RunImpl() = 0;
	virtual void Complete();
	virtual void Failure(const std::exception& exc);

	void		 PowerUp();
	void		 PowerDone();

	/**
	 * Clean up resources that this command allocates.
	 */
	virtual void Cleanup();

	PopClient&						m_client;
	std::string						m_stayAwakeReason;
	PowerUser						m_powerUser;
	MojRefCountedPtr<PowerManager>	m_powerManager;
};

#endif /* POPCLIENTCOMMAND_H_ */
