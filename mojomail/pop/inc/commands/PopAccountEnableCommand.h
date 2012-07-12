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

#ifndef POPACCOUNTENABLECOMMAND_H_
#define POPACCOUNTENABLECOMMAND_H_

#include "PopClient.h"
#include "commands/PopClientCommand.h"
#include "data/PopAccountAdapter.h"
#include "data/PopFolderAdapter.h"
#include "data/EmailAccountAdapter.h"
#include "PopDefs.h"
#include <boost/foreach.hpp>

class PopAccountEnableCommand : public PopClientCommand
{
public:
	PopAccountEnableCommand(PopClient& client, MojServiceMessage* msg, MojObject& payload);
	virtual ~PopAccountEnableCommand();
	virtual void RunImpl();

private:

	void UpdateAccount();
	void FindSpecialFolders();
	void CreatePopFolders();
	void EnableSmtpAccount();
	void UpdateMissingCredentialsSyncStatus();
	void CreateOutboxWatch();
	void CreateOutboxWatchActivity();
	void CreateMoveEmailWatch();

	MojErr 	UpdateMissingCredentialsSyncStatusResponse();
	MojErr 	UpdateAccountResponse(MojObject& response, MojErr err);
	MojErr 	FindSpecialFoldersResponse(MojObject& response, MojErr err);
	MojErr 	ReserveIdsResponse(MojObject& response, MojErr err);
	MojErr 	CreatePopFoldersResponse(MojObject& response, MojErr err);
	MojErr 	SetSpecialFoldersOnAccountResponse(MojObject& response, MojErr err);
	MojErr	SmtpAccountEnabledResponse(MojObject& response, MojErr err);
	MojErr 	GetMailAccountResponse(MojObject& response, MojErr err);
	MojErr 	CreateOutboxWatchResponse(MojObject& response, MojErr err);
	MojErr	CreateMoveEmailWatchResponse(MojObject& response, MojErr err);
	void 	Done();

	void   	AddPopFolder(MojObject::ObjectVec& folders, const char* displayName, bool favorite);

	static inline bool IsValidId(const MojObject& obj) { return !obj.undefined() && !obj.null(); }

	boost::shared_ptr<PopAccount>						m_account;
	MojObject											m_accountId;
	MojRefCountedPtr<MojServiceMessage>					m_msg;
	MojObject											m_payload;
	bool												m_needsInbox;
	bool												m_needsDrafts;
	bool											  	m_needsOutbox;
	bool												m_needsSentFolder;
	bool		  										m_needsTrashFolder;
	bool 												m_needsInitialSync;
	std::string											m_capabilityProvider;
	std::string  										m_busAddress;
	MojRefCountedPtr<SyncStateUpdater>					m_syncStateUpdater;
	MojDbClient::Signal::Slot<PopAccountEnableCommand>	m_updateAccountSlot;
	MojDbClient::Signal::Slot<PopAccountEnableCommand>	m_getFoldersSlot;
	MojDbClient::Signal::Slot<PopAccountEnableCommand> 	m_createSlot;
	MojDbClient::Signal::Slot<PopAccountEnableCommand>	m_getMailAccountSlot;
	MojDbClient::Signal::Slot<PopAccountEnableCommand>	m_createOutboxWatchSlot;
	MojDbClient::Signal::Slot<PopAccountEnableCommand>	m_createMoveEmailWatchSlot;
	MojDbClient::Signal::Slot<PopAccountEnableCommand>	m_smtpAccountEnabledSlot;
	MojSignal<>::Slot<PopAccountEnableCommand>			m_updateMissingCredentialsResponseSlot;
	MojObject											m_idArray;
	MojObject											m_transportObj;
};

#endif /* NEGOTIATETLSCOMMAND_H_ */
