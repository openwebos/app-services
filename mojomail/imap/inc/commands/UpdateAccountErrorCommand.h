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

#ifndef UPDATEACCOUNTERRORCOMMAND_H_
#define UPDATEACCOUNTERRORCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "data/EmailAccount.h"
#include "db/MojDbClient.h"
#include "CommonErrors.h"

class UpdateAccountErrorCommand : public ImapClientCommand
{
public:
	UpdateAccountErrorCommand(ImapClient& client, const MailError::ErrorInfo& error);
	virtual ~UpdateAccountErrorCommand();

	void RunImpl();

protected:

	void UpdateAccountError();
	MojErr UpdateAccountErrorResponse(MojObject& response, MojErr err);

	void Done();

	MailError::ErrorInfo	m_error;

	MojDbClient::Signal::Slot<UpdateAccountErrorCommand>		m_updateAccountErrorSlot;

};

#endif /* UPDATEACCOUNTERRORCOMMAND_H_ */
