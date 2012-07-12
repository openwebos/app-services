// @@@LICENSE
//
//      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef UPDATEFOLDERACTIVITIESCOMMAND_H_
#define UPDATEFOLDERACTIVITIESCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "db/MojDbClient.h"
#include <vector>

class ActivitySet;
class DeleteActivitiesCommand;

class UpdateFolderActivitiesCommand : public ImapClientCommand
{
public:
	UpdateFolderActivitiesCommand(ImapClient& client, const MojObject::ObjectVec& folderIds);
	virtual ~UpdateFolderActivitiesCommand();

	void Status(MojObject& status) const;

protected:
	void RunImpl();

	void GetSomeFolders();
	MojErr GetSomeFoldersResponse(MojObject& response, MojErr err);

	void GetAllFolders();
	MojErr GetAllFoldersResponse(MojObject& response, MojErr err);

	void ClearFolderActivities();
	MojErr ClearFolderActivitiesDone();

	void UpdateActivities();
	MojErr ActivitiesUpdated();

	const MojObject::ObjectVec	m_folderIds;
	MojDbQuery::Page			m_folderPage;
	std::vector<MojObject>		m_folders;

	MojRefCountedPtr<ActivitySet>				m_activitySet;
	MojRefCountedPtr<DeleteActivitiesCommand>	m_deleteActivitiesCommand;

	MojDbClient::Signal::Slot<UpdateFolderActivitiesCommand>	m_getSomeFoldersSlot;
	MojDbClient::Signal::Slot<UpdateFolderActivitiesCommand>	m_getAllFoldersSlot;
	MojSignal<>::Slot<UpdateFolderActivitiesCommand>			m_deleteActivitiesSlot;
	MojSignal<>::Slot<UpdateFolderActivitiesCommand>			m_updatedSlot;
};

#endif /* UPDATEFOLDERACTIVITIESCOMMAND_H_ */
