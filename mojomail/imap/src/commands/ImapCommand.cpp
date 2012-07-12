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

#include "commands/ImapCommand.h"
#include "client/ImapSession.h"
#include "commands/ImapCommandResult.h"
#include <sstream>
#include "client/SyncSession.h"
#include "ImapPrivate.h"

using namespace std;

#ifdef __GNUC__
#include <cxxabi.h>
#endif

ImapCommand::ImapCommand(Listener& listener, Command::Priority priority, MojLogger& logger)
: Command(listener, priority),
  m_log(logger), m_lastFunction(NULL), m_createTime(time(NULL)), m_startTime(0), m_running(false)
{
}

ImapCommand::~ImapCommand()
{
}

bool ImapCommand::PrepareToRun()
{
	return true;
}

void ImapCommand::RunIfReady()
{
	if(PrepareToRun()) {
		RunImpl();
	}
}

void ImapCommand::Run()
{
	m_running = true;
	MojLogInfo(m_log, "running command %s", Describe().c_str());
	m_startTime = time(NULL);

	try {
		RunIfReady();
	} catch(const exception& e) {
		Failure(e);
	} catch(...) {
		Failure(UNKNOWN_EXCEPTION);
	}
}

const MojRefCountedPtr<ImapCommandResult>& ImapCommand::GetResult()
{
	if(m_result.get()) {
		return m_result;
	} else {
		m_result.reset(new ImapCommandResult());
		return m_result;
	}
}

void ImapCommand::Run(MojSignal<>::SlotRef slot)
{
	if(m_result.get()) {
		m_result->ConnectDoneSlot(slot);
	} else {
		m_result.reset(new ImapCommandResult(slot));
	}

	Run();
}

void ImapCommand::SetResult(const MojRefCountedPtr<ImapCommandResult>& result)
{
	m_result = result;
}

void ImapCommand::Complete()
{
	m_running = false;

	Command::Complete();

	if(m_result.get()) {
		m_result->Done();
	}

	Cleanup();
}

void ImapCommand::Cleanup()
{
}

void ImapCommand::Failure(const exception& exc)
{
	MojLogError(m_log, "Exception in command %s: %s", Describe().c_str(), exc.what());

	if(m_result.get()) {
		m_result->SetException(exc);
	}

	Complete();
}

void ImapCommand::Cancel()
{
	// Note: most IMAP protocol commands can't be cancelled while in progress.
	// Cancel could still be used for cancelling opening a connection.
	// Cancelling attachment downloads could be implemented by closing the socket.
	Cancel(CancelType_Unknown);
}

bool ImapCommand::Cancel(CancelType type)
{
	if(IsRunning()) {
		// Don't cancel
		return false;
	} else {
		Complete();

		return true;
	}
}

MailError::ErrorInfo ImapCommand::GetCancelErrorInfo(CancelType cancelType)
{
	switch (cancelType) {
	case CancelType_Unknown:
		return MailError::ErrorInfo(MailError::INTERNAL_ERROR);
	case CancelType_NoAccount:
		return MailError::ErrorInfo(MailError::INTERNAL_ACCOUNT_MISCONFIGURED);
	case CancelType_NoConnection:
		return MailError::ErrorInfo(MailError::CONNECTION_FAILED);
	case CancelType_Shutdown:
		return MailError::ErrorInfo(MailError::INTERNAL_ERROR, "shutting down");
	}

	return MailError::ErrorInfo(MailError::INTERNAL_ERROR);
}

string ImapCommand::Describe() const
{
	return GetClassName();
}

void ImapCommand::Status(MojObject& status) const
{
	MojErr err;

	err = status.putString("class", GetClassName().c_str());
	ErrorToException(err);

	if(m_lastFunction != NULL) {
		err = status.putString("lastFunction", m_lastFunction);
		ErrorToException(err);
	}

	if(m_startTime > 0) {
		err = status.put("startTime", (MojInt64) m_startTime);
		ErrorToException(err);
	}
}
