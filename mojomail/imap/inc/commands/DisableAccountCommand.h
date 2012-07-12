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

#ifndef DISABLEACCOUNTCOMMAND_H_
#define DISABLEACCOUNTCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "commands/PurgeEmailsCommand.h"
#include "core/MojServiceRequest.h"
#include "db/MojDbClient.h"

class MojServiceMessage;
class DeleteActivitiesCommand;

class DisableAccountCommand : public ImapClientCommand
{
public:
	DisableAccountCommand(ImapClient& client, const MojRefCountedPtr<MojServiceMessage>& msg, const MojObject& payload);
	virtual ~DisableAccountCommand();

	void RunImpl();

protected:
	void GetFolders();
	MojErr GetFoldersResponse(MojObject& response, MojErr err);

	void PurgeNextFolderEmails();
	MojErr PurgeEmailsDone();

	void DeleteFolders();
	MojErr DeleteFoldersResponse(MojObject& response, MojErr err);

	void DeleteActivities();
	MojErr DeleteActivitiesDone();

	void UpdateAccount();
	MojErr UpdateAccountResponse(MojObject& response, MojErr err);

	MojErr SmtpAccountDisabledResponse(MojObject& response, MojErr err);

	void Done();

	void Failure(const std::exception& e);

	MojRefCountedPtr<MojServiceMessage>	m_msg;
	MojObject							m_payload;

	MojObject::ObjectVec m_folderIds;
	MojObject::ObjectVec::ConstIterator m_folderIdIterator;

	MojRefCountedPtr<PurgeEmailsCommand>		m_purgeCommand;
	MojRefCountedPtr<DeleteActivitiesCommand>	m_deleteActivitiesCommand;

	MojDbQuery::Page	m_folderPage;

	MojDbClient::Signal::Slot<DisableAccountCommand>			m_getFoldersSlot;
	MojSignal<>::Slot<DisableAccountCommand>					m_purgeEmailsSlot;
	MojDbClient::Signal::Slot<DisableAccountCommand>			m_deleteFoldersSlot;
	MojSignal<>::Slot<DisableAccountCommand>					m_deleteActivitiesSlot;
	MojDbClient::Signal::Slot<DisableAccountCommand>			m_updateAccountSlot;
	MojDbClient::Signal::Slot<DisableAccountCommand>			m_smtpAccountDisabledSlot;
};

#endif /* DISABLEACCOUNTCOMMAND_H_ */
