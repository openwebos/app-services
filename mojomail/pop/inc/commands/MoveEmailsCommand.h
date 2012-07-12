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

#ifndef MOVEEMAILSCOMMAND_H_
#define MOVEEMAILSCOMMAND_H_

#include "activity/Activity.h"
#include "commands/PopClientCommand.h"
#include "db/MojDbClient.h"

class MoveEmailsCommand : public PopClientCommand
{
public:
	MoveEmailsCommand(PopClient& client, MojServiceMessage* msg, MojObject& payload);
	virtual ~MoveEmailsCommand();
	virtual void RunImpl();

private:
	void 	SentEmailsQuery();
	MojErr 	SentEmailsQueryResponse(MojObject& response, MojErr err);

	MojErr	ActivityUpdate(Activity* activity, Activity::EventType event);
	MojErr 	ActivityError(Activity* activity, Activity::ErrorType error, const std::exception& exc);
	MojErr	FolderUpdateResponse(MojObject& response, MojErr err);
	MojErr 	CreateOutboxWatchResponse(MojObject& response, MojErr err);

	PopClient&										m_client;
	MojRefCountedPtr<MojServiceMessage>				m_msg;
	MojObject										m_payload;
	MojDbClient::Signal::Slot<MoveEmailsCommand>	m_sentEmailsQuerySlot;
	Activity::UpdateSignal::Slot<MoveEmailsCommand>	m_activityUpdateSlot;
	Activity::ErrorSignal::Slot<MoveEmailsCommand>	m_activityErrorSlot;
	MojDbClient::Signal::Slot<MoveEmailsCommand>	m_folderUpdateSlot;

	MojDbQuery::Page 								m_sentEmailsPage;
	MojRefCountedPtr<Activity>						m_activity;
};

#endif /* NEGOTIATETLSCOMMAND_H_ */
