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

#ifndef SYNCFOLDERCOMMAND_H_
#define SYNCFOLDERCOMMAND_H_

#include "activity/Activity.h"
#include "commands/PopClientCommand.h"
#include "core/MojObject.h"
#include "core/MojServiceRequest.h"
#include "PopClient.h"

class SyncFolderCommand : public PopClientCommand
{
public:
	SyncFolderCommand(PopClient& client, const MojObject& payload);
	virtual ~SyncFolderCommand();

	virtual void RunImpl();
private:
	MojErr		 OutboxSyncResponse(MojObject& response, MojErr err);
	void		 SyncFolder();
	void		 StartFolderSync(ActivityPtr activity);
	MojErr		 SyncSessionCompletedResponse();

	MojObject 	m_payload;
	MojObject	m_folderId;
	ActivityPtr	m_activity;
	bool		m_force;

	MojDbClient::Signal::Slot<SyncFolderCommand>	m_outboxSyncResponseSlot;
	MojSignal<>::Slot<SyncFolderCommand>			m_syncSessionCompletedSlot;
};

#endif /* SYNCFOLDERCOMMAND_H_ */
