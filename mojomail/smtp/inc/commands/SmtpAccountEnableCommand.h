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

#ifndef SMTPACCOUNTENABLECOMMAND_H_
#define SMTPACCOUNTENABLECOMMAND_H_

#include "commands/SmtpCommand.h"
#include "core/MojServiceRequest.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbClient.h"

class SmtpClient;

class SmtpAccountEnableCommand : public SmtpCommand
{
public:
	SmtpAccountEnableCommand(SmtpClient& client, const MojRefCountedPtr<MojServiceMessage>& msg);
	virtual ~SmtpAccountEnableCommand();

protected:
	void RunImpl();

	void GetMailAccount();
	MojErr GetMailAccountResponse(MojObject& response, MojErr err);

	void CreateSmtpConfigWatch();
	MojErr CreateSmtpConfigWatchResponse(MojObject& response, MojErr err);

	void CreateOutboxWatch();
	MojErr CreateOutboxWatchResponse(MojObject& response, MojErr err);

	void Failure(const std::exception& e);
	void Done();

	SmtpClient&		m_client;
	MojRefCountedPtr<MojServiceMessage>	m_msg;

	MojInt64		m_accountRev;
	MojObject		m_outboxFolderId;

	MojDbClient::Signal::Slot<SmtpAccountEnableCommand>					m_getAccountSlot;
	MojServiceRequest::ReplySignal::Slot<SmtpAccountEnableCommand>		m_createSmtpConfigWatchSlot;
	MojServiceRequest::ReplySignal::Slot<SmtpAccountEnableCommand>		m_createOutboxWatchSlot;
};

#endif /* SMTPACCOUNTENABLECOMMAND_H_ */
