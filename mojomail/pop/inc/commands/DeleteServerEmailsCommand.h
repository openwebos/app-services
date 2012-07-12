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

#ifndef DELETESERVEREMAILSCOMMAND_H_
#define DELETESERVEREMAILSCOMMAND_H_

#include "commands/DeleCommand.h"
#include "commands/PopSessionCommand.h"
#include "commands/ReconcileEmailsCommand.h"
#include "core/MojObject.h"
#include "data/PopAccount.h"
#include "data/UidCache.h"
#include "data/UidMap.h"
#include "db/MojDbClient.h"

class DeleteServerEmailsCommand : public PopSessionCommand
{
public:
	DeleteServerEmailsCommand(PopSession& session,
			const ReconcileEmailsCommand::LocalDeletedEmailsVec& localDeletedEmailUids,
			UidCache& uidCache);
	virtual ~DeleteServerEmailsCommand();

	virtual void 	RunImpl();
	MojErr			DeleteEmailResponse();
private:
	void			DeleteNextEmail();
	void			NextDeletedEmail();
	void			DeleteServerEmail();
	void 			MoveDeletedEmailToTrashFolder();
	MojErr 			MoveEmailToTrashFolderResponse(MojObject& response, MojErr err);

	const boost::shared_ptr<PopAccount>				m_account;
	const boost::shared_ptr<UidMap>&				m_uidMapPtr;
	const ReconcileEmailsCommand::LocalDeletedEmailsVec& m_localDeletedEmailUids;
	UidCache& 										m_uidCache;
	int												m_uidNdx;
	ReconcileEmailsCommand::LocalDeletedEmailInfo	m_currDeletedEmail;
	MojRefCountedPtr<DeleCommand> 					m_delServerEmailCommand;
	MojRefCountedPtr<PopCommandResult>				m_delServerResult;
	MojSignal<>::Slot<DeleteServerEmailsCommand>	m_deleteEmailResponseSlot;
	MojDbClient::Signal::Slot<DeleteServerEmailsCommand> 	m_moveDeletedEmailResponseSlot;
};

#endif /* DELETESERVEREMAILSCOMMAND_H_ */
