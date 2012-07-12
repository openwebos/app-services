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

#ifndef IDLECOMMAND_H_
#define IDLECOMMAND_H_

#include "commands/BaseIdleCommand.h"
#include "protocol/ImapResponseParser.h"
#include "core/MojObject.h"
#include "activity/Activity.h"

class SyncEmailsCommand;

class IdleCommand : public BaseIdleCommand
{
public:
	IdleCommand(ImapSession& session, const MojObject& folderId);
	virtual ~IdleCommand();

	void RunImpl();

	// Sends DONE command to server to exit from the IDLE state
	void EndIdle();

	void Status(MojObject& status) const;
	std::string Describe() const;

protected:
	void BuildUIDMap();
	MojErr SyncResponse();

	void ScheduleWakeup();
	MojErr WakeupActivityUpdate(Activity* activity, Activity::EventType);
	MojErr WakeupActivityError(Activity* activity, Activity::ErrorType, const std::exception& e);

	void Idle();
	MojErr IdleContinuation();
	MojErr IdleResponse();

	virtual void Failure(const std::exception& e);

	static const int IDLE_WAKEUP_SECONDS;
	static const int IDLE_TIMEOUT_SECONDS;

	MojObject	m_folderId;
	ActivityPtr	m_wakeupActivity;

	MojRefCountedPtr<ImapResponseParser>	m_parser;
	MojRefCountedPtr<SyncEmailsCommand>		m_syncCommand;

	MojSignal<>::Slot<IdleCommand>						m_syncSlot;
	MojSignal<>::Slot<IdleCommand>						m_continuationSlot;
	ImapResponseParser::DoneSignal::Slot<IdleCommand>	m_doneSlot;
	Activity::UpdateSignal::Slot<IdleCommand>			m_wakeupActivityUpdateSlot;
	Activity::ErrorSignal::Slot<IdleCommand>			m_wakeupActivityErrorSlot;
};

#endif /* IDLECOMMAND_H_ */
