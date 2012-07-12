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

#include "commands/IdleCommand.h"
#include "ImapPrivate.h"
#include "client/ImapSession.h"
#include "client/FolderSession.h"
#include "commands/SyncEmailsCommand.h"
#include "commands/ImapCommandResult.h"
#include "activity/Activity.h"
#include "activity/ActivityBuilder.h"
#include "data/ImapAccount.h"
#include "activity/ImapActivityFactory.h"

const int IdleCommand::IDLE_WAKEUP_SECONDS = 29 * 60; // 29 minute wakeup
const int IdleCommand::IDLE_TIMEOUT_SECONDS = 30 * 60; // 30 minute timeout waiting for response

IdleCommand::IdleCommand(ImapSession& session, const MojObject& folderId)
: BaseIdleCommand(session),
  m_folderId(folderId),
  m_syncSlot(this, &IdleCommand::SyncResponse),
  m_continuationSlot(this, &IdleCommand::IdleContinuation),
  m_doneSlot(this, &IdleCommand::IdleResponse),
  m_wakeupActivityUpdateSlot(this, &IdleCommand::WakeupActivityUpdate),
  m_wakeupActivityErrorSlot(this, &IdleCommand::WakeupActivityError)
{
}

IdleCommand::~IdleCommand()
{
}

void IdleCommand::RunImpl()
{
	CommandTraceFunction();

	boost::shared_ptr<FolderSession> folderSession = m_session.GetFolderSession();
	if(folderSession.get() && folderSession->HasUIDMap()) {
		// Already have a UID map
		ScheduleWakeup();
	} else {
		// Need a UID map before we can do anything
		BuildUIDMap();
	}
}

void IdleCommand::BuildUIDMap()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "need to sync UID list before entering idle");

	// Run sync command
	m_syncCommand.reset(new SyncEmailsCommand(m_session, m_folderId));
	m_syncCommand->Run(m_syncSlot);
}

MojErr IdleCommand::SyncResponse()
{
	CommandTraceFunction();

	try {
		m_syncCommand->GetResult()->CheckException();
		m_syncCommand.reset();

		ScheduleWakeup();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void IdleCommand::ScheduleWakeup()
{
	CommandTraceFunction();

	MojObject accountId = m_session.GetAccount()->GetId();

	ImapActivityFactory factory;
	ActivityBuilder ab;

	factory.BuildIdleWakeup(ab, accountId, m_folderId, IDLE_WAKEUP_SECONDS);

	m_wakeupActivity = Activity::PrepareNewActivity(ab);
	m_wakeupActivity->SetSlots(m_wakeupActivityUpdateSlot, m_wakeupActivityErrorSlot);
	m_wakeupActivity->Create(m_session.GetBusClient());

	// FIXME: wait until activity has been created
	Idle();
}

void IdleCommand::Idle()
{
	CommandTraceFunction();

	m_parser.reset(new ImapResponseParser(m_session, m_doneSlot));
	m_parser->SetContinuationResponseSlot(m_continuationSlot);
	m_session.SendRequest("IDLE", m_parser);
}

MojErr IdleCommand::WakeupActivityUpdate(Activity* activity, Activity::EventType eventType)
{
	if(eventType == Activity::StartEvent) {
		EndIdle();
	}

	return MojErrNone;
}

MojErr IdleCommand::WakeupActivityError(Activity* activity, Activity::ErrorType errorType, const exception& e)
{
	MojLogWarning(m_log, "failed to create idle wakeup activity: %s", e.what());
	return MojErrNone;
}

// The server will send us something like +idling to indicate that it has started idling
MojErr IdleCommand::IdleContinuation()
{
	CommandTraceFunction();

	try {
		m_started = true;

		// Let the session know that we're now successfully idling
		m_session.IdleStarted();

		// Update the request timeout
		// Note: this timeout should never happen, since the wakeup should occur first
		m_session.UpdateRequestTimeout(m_parser, IDLE_TIMEOUT_SECONDS);
	} CATCH_AS_FAILURE

	return MojErrNone;
}

MojErr IdleCommand::IdleResponse()
{
	CommandTraceFunction();

	try {
		// End activity
		m_wakeupActivityUpdateSlot.cancel();
		m_wakeupActivityErrorSlot.cancel();
		m_wakeupActivity->End(m_session.GetBusClient());

		// Check if IDLE command actually worked
		if(m_parser->GetStatus() == NO || m_parser->GetStatus() == BAD) {
			// Disable IDLE from capabilities if the server said it was bad
			m_session.GetCapabilities().RemoveCapability(Capabilities::IDLE);
		}

		// Check again for any exceptions
		m_parser->CheckStatus();

		m_session.IdleDone();
		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void IdleCommand::Failure(const exception& e)
{
	m_session.IdleError(e, false);
	m_session.IdleDone();
	ImapSessionCommand::Failure(e);
}

void IdleCommand::EndIdle()
{
	CommandTraceFunction();

	if(!m_endingIdle) {
		m_endingIdle = true;

		// Tell the server to break out of IDLE
		// It should respond by completing the original IDLE command request
		MojLogDebug(m_log, "sending DONE to break out of idle");

		try {
			if(m_session.GetOutputStream().get() != NULL) {
				m_session.GetOutputStream()->Write("DONE\r\n");
				m_session.GetOutputStream()->Flush();

				if(m_session.GetLineReader().get()) {
					// Give it 30 seconds to respond to the DONE
					m_session.GetLineReader()->SetTimeout(30);
				}
			} else {
				throw MailException("no connection output stream", __FILE__, __LINE__);
			}
		} CATCH_AS_FAILURE
	} else {
		MojLogInfo(m_log, "already ending idle");
	}
}

void IdleCommand::Status(MojObject& status) const
{
	MojErr err;

	ImapSessionCommand::Status(status);

	if(m_endingIdle) {
		err = status.put("endingIdle", true);
		ErrorToException(err);
	}

	if(m_wakeupActivity.get()) {
		MojObject activityStatus;
		m_wakeupActivity->Status(activityStatus);
		err = status.put("wakeupActivity", activityStatus);
		ErrorToException(err);
	}

	if(m_syncCommand.get()) {
		MojObject commandStatus;
		m_syncCommand->Status(commandStatus);
		err = status.put("syncCommand", commandStatus);
		ErrorToException(err);
	}
}

std::string IdleCommand::Describe() const
{
	return ImapSessionCommand::Describe(m_folderId);
}
