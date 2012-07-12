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

#ifndef CHECKOUTBOXCOMMAND_H_
#define CHECKOUTBOXCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "db/MojDbClient.h"
#include "activity/Activity.h"
#include "client/SyncParams.h"

class ActivitySet;

class CheckOutboxCommand : public ImapClientCommand
{
public:
	CheckOutboxCommand(ImapClient& client, SyncParams syncParams);
	virtual ~CheckOutboxCommand();

protected:
	virtual void RunImpl();

	void GetSentEmails();
	MojErr GetSentEmailsResponse(MojObject& response, MojErr err);

	void GetEmailsToDelete();
	MojErr GetEmailsToDeleteResponse(MojObject& response, MojErr err);

	MojErr PurgeResponse(MojObject& response, MojErr err);

	void EndActivities();
	MojErr EndActivitiesDone();

	SyncParams						m_syncParams;
	MojRefCountedPtr<ActivitySet>	m_activitySet;

	MojDbQuery::Page m_sentEmailsPage;

	MojDbClient::Signal::Slot<CheckOutboxCommand>	m_getSentEmailsSlot;
	MojDbClient::Signal::Slot<CheckOutboxCommand>	m_getToDeleteSlot;
	MojDbClient::Signal::Slot<CheckOutboxCommand>	m_purgeSlot;
	MojSignal<>::Slot<CheckOutboxCommand>			m_endActivitiesSlot;
};

#endif /* CHECKOUTBOXCOMMAND_H_ */
