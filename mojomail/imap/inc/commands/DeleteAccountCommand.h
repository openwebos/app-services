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

#ifndef DELETEACCOUNTCOMMAND_H_
#define DELETEACCOUNTCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbClient.h"

class MojServiceMessage;

class DeleteAccountCommand : public ImapClientCommand
{
public:
	DeleteAccountCommand(ImapClient& client, const MojRefCountedPtr<MojServiceMessage>& msg, const MojObject& payload);
	virtual ~DeleteAccountCommand();

	void RunImpl();

protected:
	void GetAccountInfo();
	MojErr GetAccountInfoResponse(MojObject& response, MojErr err);

	void DeleteAccount();
	MojErr DeleteAccountResponse(MojObject& response, MojErr err);

	void DeleteSmtpAccount();
	MojErr SmtpAccountDeletedResponse(MojObject& response, MojErr err);

	void DeleteYahooAccount();
	MojErr YahooAccountDeletedResponse(MojObject& response, MojErr err);

	void Done();

	MojRefCountedPtr<MojServiceMessage>	m_msg;
	MojObject							m_payload;
	bool								m_isYahoo;

	MojServiceRequest::ReplySignal::Slot<DeleteAccountCommand> 	m_getAccountSlot;
	MojDbClient::Signal::Slot<DeleteAccountCommand>		m_deleteAccountSlot;
	MojDbClient::Signal::Slot<DeleteAccountCommand>		m_smtpAccountDeletedSlot;
	MojDbClient::Signal::Slot<DeleteAccountCommand>		m_yahooAccountDeletedSlot;
};

#endif /* DELETEACCOUNTCOMMAND_H_ */
