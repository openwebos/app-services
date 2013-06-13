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

#ifndef DELETELOCALEMAILSCOMMAND_H_
#define DELETELOCALEMAILSCOMMAND_H_

#include "commands/PopSessionCommand.h"
#include "core/MojObject.h"
#include "data/OldEmailsCache.h"
#include "data/PopAccount.h"
#include "data/PopEmail.h"
#include "data/UidMap.h"
#include "db/MojDbClient.h"

class DeleteLocalEmailsCommand : public PopSessionCommand
{
public:
	DeleteLocalEmailsCommand(PopSession& session,
			const MojObject::ObjectVec& serverDeletedEmailIds,
			const MojObject::ObjectVec& oldEmailIds);
	virtual ~DeleteLocalEmailsCommand();

	virtual void 	RunImpl();
	MojErr			DeleteLocalEmailsResponse(MojObject& response, MojErr err);
private:
	void			DetermineDeletedEmails();
	void			DeleteLocalEmails(const MojObject::ObjectVec& delEmails);

	boost::shared_ptr<PopAccount>						m_account;
	const MojObject::ObjectVec&							m_serverDeletedEmailIds;
	const MojObject::ObjectVec&							m_oldEmailIds;
	MojObject::ObjectVec								m_deletedEmailIds;
	MojDbClient::Signal::Slot<DeleteLocalEmailsCommand> m_deleteLocalEmailsResponseSlot;
};
#endif /* DELETELOCALEMAILSCOMMAND_H_ */
