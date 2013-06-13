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

#ifndef SCHEDULERETRYCOMMAND_H_
#define SCHEDULERETRYCOMMAND_H_

#include "commands/PopClientCommand.h"
#include "core/MojObject.h"
#include "core/MojServiceRequest.h"
#include "data/EmailAccount.h"
#include "db/MojDbClient.h"

class DeleteActivitiesCommand;

/**
 * Schedules or cancels the retry for a folder
 */
class ScheduleRetryCommand : public PopClientCommand
{
public:
	ScheduleRetryCommand(PopClient& client, const MojObject& folderId, EmailAccount::AccountError err);
	virtual ~ScheduleRetryCommand();

protected:
	void RunImpl();

	void ScheduleRetry();
	MojErr ScheduleRetryResponse(MojObject& response, MojErr err);

	void CancelRetry();
	MojErr CancelRetryResponse(MojObject& response, MojErr err);

	void UpdateAccount();
	MojErr UpdateAccountResponse(MojObject& response, MojErr err);

	void CancelActivities();
	MojErr CancelActivitiesDone();

	static int INITIAL_RETRY_SECONDS;
	static int MAX_RETRY_SECONDS;
	static float RETRY_MULTIPLIER;

	MojObject					m_folderId;
	EmailAccount::AccountError	m_accntErr;

	MojRefCountedPtr<DeleteActivitiesCommand> m_deleteActivitiesCommand;

	MojServiceRequest::ReplySignal::Slot<ScheduleRetryCommand>	m_scheduleRetrySlot;
	MojSignal<>::Slot<ScheduleRetryCommand>						m_deleteActivitiesSlot;
	MojDbClient::Signal::Slot<ScheduleRetryCommand>				m_updateAccountSlot;
};


#endif /* SCHEDULERETRYCOMMAND_H_ */
