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

#ifndef CREATEDEFAULTFOLDERSCOMMAND_H_
#define CREATEDEFAULTFOLDERSCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "data/ImapAccount.h"
#include "data/ImapFolder.h"
#include "db/MojDbClient.h"
#include "ImapCoreDefs.h"
#include <boost/regex.hpp>
#include <vector>

class CreateDefaultFoldersCommand : public ImapClientCommand
{
	typedef boost::shared_ptr<ImapFolder> ImapFolderPtr;
	typedef boost::shared_ptr<Folder> FolderPtr;
	typedef boost::shared_ptr<ImapAccount> ImapAccountPtr;

public:
	CreateDefaultFoldersCommand(ImapClient& client);
	virtual ~CreateDefaultFoldersCommand();

	void RunImpl();

protected:
	void FindSpecialFolders();
	MojErr FindSpecialFoldersResponse(MojObject& response, MojErr err);

	void CreateFolders();
	MojErr CreateFoldersResponse(MojObject& response, MojErr err);

	void UpdateAccount();
	MojErr UpdateAccountResponse(MojObject& response, MojErr err);

	void SetupOutboxWatchActivity();
	MojErr SetupOutboxWatchActivityResponse(MojObject& response, MojErr err);

	void Done();

	static inline bool IsValidId(const MojObject& obj) { return !obj.undefined() && !obj.null(); }

	MojObject						m_accountId;

	bool	m_needsInbox;
	bool	m_needsOutbox;

	MojDbClient::Signal::Slot<CreateDefaultFoldersCommand>			m_getFoldersSlot;
	MojDbClient::Signal::Slot<CreateDefaultFoldersCommand>			m_createFoldersSlot;
	MojDbClient::Signal::Slot<CreateDefaultFoldersCommand>			m_updateAccountSlot;
	MojDbClient::Signal::Slot<CreateDefaultFoldersCommand> 			m_setupOutboxWatchActivitySlot;

};

#endif /* CREATEDEFAULTFOLDERSCOMMAND_H_ */
