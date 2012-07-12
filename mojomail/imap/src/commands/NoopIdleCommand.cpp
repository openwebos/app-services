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

#include "commands/NoopIdleCommand.h"
#include "activity/ActivityBuilder.h"
#include "activity/ActivitySet.h"
#include "activity/ImapActivityFactory.h"
#include "client/ImapSession.h"
#include "ImapPrivate.h"

/**
 * If the connection is idle for longer than this period, issue a NOOP to check if the connection is still
 * good before exiting idle.
 */
const int NoopIdleCommand::NOOP_THRESHOLD = 20; // 20 seconds

NoopIdleCommand::NoopIdleCommand(ImapSession& session, const MojObject& folderId, int timeoutSeconds)
: BaseIdleCommand(session),
  m_folderId(folderId),
  m_timeoutSeconds(timeoutSeconds),
  m_timeStarted(0),
  m_timeExpired(false),
  m_wakeupActivityUpdateSlot(this, &NoopIdleCommand::WakeupActivityUpdate),
  m_wakeupActivityErrorSlot(this, &NoopIdleCommand::WakeupActivityError),
  m_noopResponseSlot(this, &NoopIdleCommand::NoopResponse)
{
}

NoopIdleCommand::~NoopIdleCommand()
{
}

void NoopIdleCommand::RunImpl()
{
	SetupTimeout();
}

void NoopIdleCommand::SetupTimeout()
{
	CommandTraceFunction();

	ImapActivityFactory factory;
	ActivityBuilder ab;

	m_timeStarted = time(NULL);

	factory.BuildIdleWakeup(ab, m_session.GetAccount()->GetAccountId(), m_folderId, m_timeoutSeconds);

	m_wakeupActivity = Activity::PrepareNewActivity(ab);
	m_wakeupActivity->SetSlots(m_wakeupActivityUpdateSlot, m_wakeupActivityErrorSlot);
	m_wakeupActivity->Create(m_session.GetBusClient());
}

MojErr NoopIdleCommand::WakeupActivityUpdate(Activity* activity, Activity::EventType eventType)
{
	if(eventType == Activity::StartEvent) {
		m_timeExpired = true;
		EndIdle();
	} else if(eventType == Activity::UpdateEvent) {
		m_session.IdleStarted();
	}

	return MojErrNone;
}

MojErr NoopIdleCommand::WakeupActivityError(Activity* activity, Activity::ErrorType errorType, const exception& e)
{
	MojLogWarning(m_log, "failed to create noop idle wakeup activity: %s", e.what());
	return MojErrNone;
}

void NoopIdleCommand::EndIdle()
{
	CommandTraceFunction();

	m_wakeupActivity->Unsubscribe();

	MojLogInfo(m_log, "ending NOOP idle");

	long duration = std::abs(time(NULL) - m_timeStarted);

	if (m_session.IsConnected() && !m_timeExpired && duration >= NOOP_THRESHOLD) {
		try {
			SendNoop();
		} CATCH_AS_FAILURE
	} else {
		m_session.IdleDone();
		Complete();
	}
}

void NoopIdleCommand::SendNoop()
{
	CommandTraceFunction();

	// issue a NOOP command to check if the connection is still alive
	m_responseParser.reset(new ImapResponseParser(m_session, m_noopResponseSlot));
	m_session.SendRequest("NOOP", m_responseParser);
}

MojErr NoopIdleCommand::NoopResponse()
{
	CommandTraceFunction();

	try {
		m_responseParser->CheckStatus();

		m_session.IdleDone();
		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void NoopIdleCommand::Failure(const exception& e)
{
	m_session.IdleDone();
	ImapSessionCommand::Failure(e);
}
