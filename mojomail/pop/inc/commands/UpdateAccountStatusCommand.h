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

#ifndef UPDATEACCOUNTSTATUSCOMMAND_H_
#define UPDATEACCOUNTSTATUSCOMMAND_H_

#include "commands/PopSessionCommand.h"
#include "core/MojObject.h"
#include "db/MojDbClient.h"

class UpdateAccountStatusCommand : public PopSessionCommand
{
public:
	UpdateAccountStatusCommand(PopSession& session,
			boost::shared_ptr<PopAccount> account,
			MailError::ErrorCode errCode,
			const std::string& errMsg);
	~UpdateAccountStatusCommand();

	virtual void 	RunImpl();
	MojErr			UpdateAccountResponse(MojObject& response, MojErr err);
private:
	void			UpdateAccount();

	boost::shared_ptr<PopAccount>							m_account;
	MailError::ErrorCode									m_errorCode;
	std::string												m_errorMessage;
	MojDbClient::Signal::Slot<UpdateAccountStatusCommand> 	m_updateResponseSlot;
};

#endif /* UPDATEACCOUNTSTATUSCOMMAND_H_ */
