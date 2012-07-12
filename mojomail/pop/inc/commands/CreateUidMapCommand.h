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

#ifndef CREATEUIDMAPCOMMAND_H_
#define CREATEUIDMAPCOMMAND_H_

#include "client/PopSession.h"
#include "commands/ListCommand.h"
#include "commands/PopSessionCommand.h"
#include "commands/UidlCommand.h"
#include "core/MojObject.h"
#include "data/UidMap.h"

class CreateUidMapCommand : public PopSessionCommand
{
public:
	CreateUidMapCommand(PopSession& session, boost::shared_ptr<UidMap>& uidMapPtr);
	~CreateUidMapCommand();

	virtual void 	RunImpl();
	MojErr 			GetUidlCommandResponse();
	MojErr			GetListCommandResponse();
private:
	boost::shared_ptr<UidMap>				m_uidMapPtr;
	MojRefCountedPtr<UidlCommand> 			m_uidlCommand;
	MojRefCountedPtr<ListCommand> 			m_listCommand;

	MojRefCountedPtr<PopCommandResult>		m_commandResult;
	MojSignal<>::Slot<CreateUidMapCommand> 	m_getUidlCommandResponseSlot;
	MojSignal<>::Slot<CreateUidMapCommand> 	m_getListCommandResponseSlot;
};

#endif /* CREATEUIDMAPCOMMAND_H_ */
