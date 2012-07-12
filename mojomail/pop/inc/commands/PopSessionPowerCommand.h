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

#ifndef POPSESSIONPOWERCOMMAND_H_
#define POPSESSIONPOWERCOMMAND_H_

#include "client/PowerManager.h"
#include "client/PowerUser.h"
#include "commands/PopSessionCommand.h"

/**
 * A power aware command class.  When this command is initialized, this class
 * will inform power manager to make the device stay away.
 */
class PopSessionPowerCommand : public PopSessionCommand
{
public:
	PopSessionPowerCommand(PopSession& session, const std::string& reason);
	~PopSessionPowerCommand();
protected:
	void		 PowerUp();
	void		 PowerDone();

	/**
	 * Clean up resources that this command allocates.
	 */
	virtual void Cleanup();

	std::string						m_stayAwakeReason;
	PowerUser						m_powerUser;
	MojRefCountedPtr<PowerManager>	m_powerManager;
};

#endif /* POPSESSIONPOWERCOMMAND_H_ */
