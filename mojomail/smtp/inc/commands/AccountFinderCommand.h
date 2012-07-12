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

#ifndef ACCOUNTFINDERCOMMAND_H_
#define ACCOUNTFINDERCOMMAND_H_

#include "commands/SmtpCommand.h"
#include "core/MojObject.h"
#include "core/MojServiceRequest.h"
#include "data/SmtpAccount.h"
#include "db/MojDbClient.h"
#include "client/SmtpSession.h"

class SmtpSession;

class AccountFinderCommand : public SmtpCommand
{
public:
	AccountFinderCommand(SmtpSession& session,
						 MojObject accountId);
	virtual ~AccountFinderCommand();

	virtual void RunImpl();
	void Cancel();
private:
	SmtpSession&						m_session;
	boost::shared_ptr<SmtpAccount>	m_account;
	MojObject						m_accountId;
	MojObject						m_accountObj;

	void	GetAccount();
	MojErr	GetAccountResponse(MojObject& response, MojErr err);
	void	GetCommonCredentials();
	MojErr	GetCommonCredentialsResponse(MojObject& response, MojErr err);
	void	GetSmtpCredentials();
	MojErr	GetSmtpCredentialsResponse(MojObject& response, MojErr err);
	void	GetSmtpAccount();
	MojErr	GetSmtpAccountResponse(MojObject& response, MojErr err);

	MojServiceRequest::ReplySignal::Slot<AccountFinderCommand> 	m_getAccountSlot;
	MojServiceRequest::ReplySignal::Slot<AccountFinderCommand> 	m_getCommonCredentialsResponseSlot;
	MojServiceRequest::ReplySignal::Slot<AccountFinderCommand> 	m_getSmtpCredentialsResponseSlot;
	    
	MojDbClient::Signal::Slot<AccountFinderCommand>				m_getSmtpAccountSlot;
};

#endif /* ACCOUNTFINDERCOMMAND_H_ */
