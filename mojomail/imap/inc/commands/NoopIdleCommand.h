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

#ifndef NOOPIDLECOMMAND_H_
#define NOOPIDLECOMMAND_H_

#include "commands/BaseIdleCommand.h"
#include "core/MojObject.h"

class ActivitySet;
class ImapResponseParser;

class NoopIdleCommand : public BaseIdleCommand
{
public:
	NoopIdleCommand(ImapSession& session, const MojObject& folderId, int timeoutSeconds);
	virtual ~NoopIdleCommand();

protected:
	static const int NOOP_THRESHOLD; // how long before we need to issue a NOOP to test if it's still alive

	void RunImpl();

	void SetupTimeout();

	MojErr WakeupActivityUpdate(Activity* activity, Activity::EventType);
	MojErr WakeupActivityError(Activity* activity, Activity::ErrorType, const std::exception& e);

	void EndIdle();

	void	SendNoop();
	MojErr	NoopResponse();

	void Failure(const std::exception& e);

	MojObject	m_folderId;
	int			m_timeoutSeconds;

	time_t		m_timeStarted;
	bool		m_timeExpired;

	ActivityPtr	m_wakeupActivity;

	MojRefCountedPtr<ImapResponseParser> m_responseParser;

	Activity::UpdateSignal::Slot<NoopIdleCommand>		m_wakeupActivityUpdateSlot;
	Activity::ErrorSignal::Slot<NoopIdleCommand>		m_wakeupActivityErrorSlot;
	MojSignal<>::Slot<NoopIdleCommand>					m_noopResponseSlot;
};

#endif /* NOOPIDLECOMMAND_H_ */
