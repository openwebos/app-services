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

#ifndef ACCOUNTFINDERCOMMAND_H_
#define ACCOUNTFINDERCOMMAND_H_

#include "commands/PopClientCommand.h"
#include "core/MojObject.h"
#include "core/MojServiceRequest.h"
#include "data/PopAccount.h"
#include "db/MojDbClient.h"
#include "PopClient.h"

class AccountFinderCommand : public PopClientCommand
{
public:
	AccountFinderCommand(PopClient& client, MojObject accountId);
	virtual ~AccountFinderCommand();

	virtual void RunImpl();
private:
	boost::shared_ptr<PopAccount>	m_account;
	MojObject						m_accountId;
	MojObject						m_accountObj;

	void	GetAccount();
	MojErr	GetAccountResponse(MojObject& response, MojErr err);
	void	GetPassword();
	MojErr	GetPasswordResponse(MojObject& response, MojErr err);
	void	GetPopAccount();
	MojErr	GetPopAccountResponse(MojObject& response, MojErr err);
	virtual void Failure(const std::exception& exc);

	MojServiceRequest::ReplySignal::Slot<AccountFinderCommand> 	m_getAccountSlot;
	MojServiceRequest::ReplySignal::Slot<AccountFinderCommand> 	m_getPasswordSlot;
	MojDbClient::Signal::Slot<AccountFinderCommand>				m_getPopAccountSlot;
};

#endif /* ACCOUNTFINDERCOMMAND_H_ */
