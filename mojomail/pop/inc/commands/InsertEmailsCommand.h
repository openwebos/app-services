// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "client/PopSession.h"
#include "commands/PopSessionCommand.h"
#include "data/PopEmail.h"
#include "db/MojDbClient.h"

#ifndef INSERTEMAILSCOMMAND_H_
#define INSERTEMAILSCOMMAND_H_

class InsertEmailsCommand : public PopSessionCommand
{
public:
	InsertEmailsCommand(PopSession& session, PopEmail::PopEmailPtrVectorPtr emails);
	virtual ~InsertEmailsCommand();

	virtual void RunImpl();
	MojErr	ReserverEmailIdsResponse(MojObject& response, MojErr err);
	MojErr	SaveEmailsResponse(MojObject& response, MojErr err);
private:
	MojErr	SaveEmails();

	PopEmail::PopEmailPtrVectorPtr 					m_emails;
	MojObject::ObjectVec							m_persistEmails;
	MojDbClient::Signal::Slot<InsertEmailsCommand>	m_reserveIdsSlot;
	MojDbClient::Signal::Slot<InsertEmailsCommand> 	m_saveEmailsSlot;
};

#endif /* INSERTEMAILSCOMMAND_H_ */
