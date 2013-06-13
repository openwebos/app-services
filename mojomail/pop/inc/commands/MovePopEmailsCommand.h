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

#ifndef MOVEPOPEMAILSCOMMAND_H_
#define MOVEPOPEMAILSCOMMAND_H_

#include <set>
#include "activity/Activity.h"
#include "data/UidCache.h"
#include "db/MojDbClient.h"
#include "commands/PopClientCommand.h"
#include "commands/LoadUidCacheCommand.h"
#include "commands/UpdateUidCacheCommand.h"

class MovePopEmailsCommand : public PopClientCommand
{
public:
	MovePopEmailsCommand(PopClient& client, MojServiceMessage* msg, MojObject& payload);
	virtual ~MovePopEmailsCommand();
	virtual void RunImpl();

private:
	void GetEmailsToMove();
	void GetUidCache();
	void SaveUidCache();

	MojErr	GetEmailsToMoveResponse(MojObject& response, MojErr err);
	MojErr	ActivityUpdate(Activity* activity, Activity::EventType event);
	MojErr 	ActivityError(Activity* activity, Activity::ErrorType error, const std::exception& exc);
	MojErr	EmailsMovedResponse(MojObject& response, MojErr err);
	MojErr	GetUidCacheResponse();
	MojErr	SaveUidCacheResponse();

	MojRefCountedPtr<MojServiceMessage>						m_msg;
	MojObject												m_payload;
	Activity::UpdateSignal::Slot<MovePopEmailsCommand>		m_activityUpdateSlot;
	Activity::ErrorSignal::Slot<MovePopEmailsCommand>		m_activityErrorSlot;
	MojDbClient::Signal::Slot<MovePopEmailsCommand>			m_getEmailsToMoveSlot;
	MojDbClient::Signal::Slot<MovePopEmailsCommand>			m_emailsMovedSlot;
	MojRefCountedPtr<PopCommandResult>						m_commandResult;
	MojRefCountedPtr<LoadUidCacheCommand>					m_loadCacheCommand;
	MojSignal<>::Slot<MovePopEmailsCommand> 				m_getUidCacheResponseSlot;
	MojRefCountedPtr<UpdateUidCacheCommand>					m_saveCacheCommand;
	MojSignal<>::Slot<MovePopEmailsCommand> 				m_saveUidCacheResponseSlot;

	MojRefCountedPtr<Activity>								m_activity;
	MojObject												m_accountId;
	std::set<std::string>									m_inboxEmailsMoved;
	UidCache												m_uidCache;
};

#endif /* MOVEPOPEMAILSCOMMAND_H_ */
