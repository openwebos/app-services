// @@@LICENSE
//
//      Copyright (c) 2010-2013 LG Electronics, Inc.
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

#ifndef DELETEFOLDERSCOMMAND_H_
#define DELETEFOLDERSCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "db/MojDbClient.h"

class DeleteActivitiesCommand;
class PurgeEmailsCommand;

class DeleteFoldersCommand : public ImapClientCommand
{
public:
	DeleteFoldersCommand(ImapClient& client, const MojObject::ObjectVec& folderIds);
	virtual ~DeleteFoldersCommand();

protected:
	void RunImpl();

	void DeleteActivities();
	MojErr DeleteActivitiesDone();

	void PurgeEmails();
	MojErr PurgeEmailsDone();

	void DeleteFolders();
	MojErr DeleteFoldersResponse(MojObject& response, MojErr err);

	MojObject::ObjectVec m_folderIds;
	MojObject::ObjectVec m_foldersToPurge;

	MojRefCountedPtr<DeleteActivitiesCommand>	m_deleteActivitiesCommand;
	MojRefCountedPtr<PurgeEmailsCommand>		m_purgeEmailsCommand;

	MojSignal<>::Slot<DeleteFoldersCommand> 			m_deleteActivitiesSlot;
	MojSignal<>::Slot<DeleteFoldersCommand> 			m_purgeSlot;
	MojDbClient::Signal::Slot<DeleteFoldersCommand>		m_deleteFoldersSlot;
};

#endif /* DELETEFOLDERSCOMMAND_H_ */
