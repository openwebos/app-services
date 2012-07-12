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

#ifndef UPDATEACCOUNTCOMMAND_H_
#define UPDATEACCOUNTCOMMAND_H_


#include "core/MojObject.h"
#include "data/ImapAccount.h"
#include "db/MojDbClient.h"
#include "ImapClientCommand.h"
#include "data/DatabaseInterface.h"
#include "commands/ImapAccountFinder.h"
#include "commands/ImapCommandResult.h"
#include "activity/Activity.h"
#include "core/MojServiceRequest.h"

class MojServiceMessage;
class ActivitySet;
class UpdateFolderActivitiesCommand;

class UpdateAccountCommand : public ImapClientCommand
{
public:
	UpdateAccountCommand(ImapClient& client, ActivityPtr activity, bool credentialsChanged);
	virtual ~UpdateAccountCommand();

	virtual void RunImpl();
	virtual void Status(MojObject& status) const;

private:
	MojObject					m_accountId;

	MojRefCountedPtr<ActivitySet>		m_activitySet;

	bool m_credentialsChanged;

	void GetAccountTransport();
	MojErr GetAccountTransportResponse();

	void UpdateFolderActivities();
	MojErr UpdateFolderActivitiesDone();

	void UpdatePreferencesWatchActivity(MojInt64 rev);

	MojErr ActivitiesEnded();

	void NotifySmtp();
	MojErr NotifySmtpResponse(MojObject& response, MojErr err);

	void Done();

	MojRefCountedPtr<ImapAccountFinder>	m_imapAccountFinder;
	MojRefCountedPtr<UpdateFolderActivitiesCommand>	m_updateFolderActivitiesCommand;

	MojSignal<>::Slot<UpdateAccountCommand>		m_getAccountTransportSlot;
	MojSignal<>::Slot<UpdateAccountCommand> 	m_updateFolderActivitiesSlot;
	MojSignal<>::Slot<UpdateAccountCommand> 	m_endActivitiesSlot;
	MojServiceRequest::ReplySignal::Slot<UpdateAccountCommand>	m_notifySmtpSlot;
};

#endif /* UPDATEACCOUNTCOMMAND_H_ */
