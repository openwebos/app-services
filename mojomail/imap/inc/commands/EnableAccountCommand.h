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

#ifndef ENABLEACCOUNTCOMMAND_H_
#define ENABLEACCOUNTCOMMAND_H_

#include "ImapClientCommand.h"
#include "core/MojServiceRequest.h"
#include "client/SyncStateUpdater.h"

class CreateDefaultFoldersCommand;
class UpdateAccountCommand;

class EnableAccountCommand : public ImapClientCommand
{
public:
	EnableAccountCommand(ImapClient& client, MojServiceMessage* msg);
	virtual ~EnableAccountCommand();

	void Status(MojObject& status) const;

	virtual bool Cancel(CancelType cancelType);

protected:
	void RunImpl();

	void CreateDefaultFolders();
	MojErr CreateDefaultFoldersDone();

	void EnableSmtpAccount();
	MojErr EnableSmtpAccountResponse(MojObject& response, MojErr err);

	void UpdateAccount();
	MojErr UpdateAccountDone();

	void UpdateMissingCredentialsSyncStatus();
	MojErr 	UpdateMissingCredentialsSyncStatusResponse();

	void Done();
	void Failure(const std::exception& e);

	MojRefCountedPtr<MojServiceMessage>				m_msg;
	std::string										m_capabilityProvider;
	std::string  									m_busAddress;
	MojRefCountedPtr<CreateDefaultFoldersCommand>	m_createDefaultFoldersCommand;
	MojRefCountedPtr<UpdateAccountCommand>			m_updateAccountCommand;
	MojRefCountedPtr<SyncStateUpdater>				m_syncStateUpdater;

	MojSignal<>::Slot<EnableAccountCommand>						m_createDefaultFoldersSlot;
	MojServiceRequest::ReplySignal::Slot<EnableAccountCommand>	m_enableSmtpAccountSlot;
	MojSignal<>::Slot<EnableAccountCommand>						m_updateAccountSlot;
	MojSignal<>::Slot<EnableAccountCommand>						m_updateMissingCredentialsResponseSlot;
};

#endif /* ENABLEACCOUNTCOMMAND_H_ */
