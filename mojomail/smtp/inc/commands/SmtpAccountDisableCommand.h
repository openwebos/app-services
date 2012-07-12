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

#ifndef SMTPACCOUNTDISABLECOMMAND_H_
#define SMTPACCOUNTDISABLECOMMAND_H_

#include "SmtpCommand.h"
#include "SmtpClient.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbClient.h"
#include <boost/regex.hpp>

class SmtpAccountDisableCommand : public SmtpCommand
{
public:
	SmtpAccountDisableCommand(SmtpClient& client, const MojRefCountedPtr<MojServiceMessage> msg, const MojObject& accountId);
	virtual ~SmtpAccountDisableCommand();

	virtual void RunImpl();
	
protected:
	
	MojErr	DeleteOutboxWatchResponse(MojObject& reponse, MojErr err);
	MojErr	DeleteAccountWatchResponse(MojObject& response, MojErr err);
	
	SmtpClient&	m_client;
	MojObject	m_accountId;
	MojServiceRequest::ReplySignal::Slot<SmtpAccountDisableCommand>		m_deleteOutboxWatchResponseSlot;
	MojServiceRequest::ReplySignal::Slot<SmtpAccountDisableCommand>		m_deleteAccountWatchResponseSlot;

 	MojString	m_outboxWatchActivityName;
 	MojString	m_accountWatchActivityName;
 	const MojRefCountedPtr<MojServiceMessage> m_msg;
};

#endif /*SMTPACCOUNTDISABLECOMMAND_H_*/
