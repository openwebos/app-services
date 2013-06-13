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


#ifndef IMAPACCOUNTFINDER_H_
#define IMAPACCOUNTFINDER_H_

#include "core/MojObject.h"
#include "core/MojServiceRequest.h"
#include "data/ImapAccount.h"
#include "db/MojDbClient.h"
#include "ImapCommand.h"
#include <memory>
#include "boost/shared_ptr.hpp"
class ImapClient;
class ImapAccountFinder : public ImapCommand
{
public:
	ImapAccountFinder(ImapClient& client,
					 MojObject accountId,
					 bool sync);
	virtual ~ImapAccountFinder();

	virtual void RunImpl();

	virtual void Failure(const std::exception& e);

private:
	ImapClient&						m_client;
	boost::shared_ptr<ImapAccount> 	m_account;
	MojObject						m_accountId;
	MojObject						m_accountObj;
	bool							m_sync;

	void	GetAccount();
	MojErr	GetAccountResponse(MojObject& response, MojErr err);
	void	GetImapAccount();
	MojErr	GetImapAccountResponse(MojObject& response, MojErr err);
	void	GetPassword();
	MojErr	GetPasswordResponse(MojObject& response, MojErr err);

	void	Done();

	MojServiceRequest::ReplySignal::Slot<ImapAccountFinder> 	m_getAccountSlot;
	MojServiceRequest::ReplySignal::Slot<ImapAccountFinder> 	m_getPasswordSlot;
	MojDbClient::Signal::Slot<ImapAccountFinder>				m_getImapAccountSlot;
};

#endif /* IMAPACCOUNTFINDER_H_ */
