// @@@LICENSE
//
//      Copyright (c) 2010-2013 LG Electronics, Inc.
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

#include "commands/ImapSyncSessionCommand.h"
#include "ImapPrivate.h"
#include "client/ImapSession.h"
#include "client/SyncSession.h"

ImapSyncSessionCommand::ImapSyncSessionCommand(ImapSession& session, const MojObject& folderId, Priority priority)
: ImapSessionCommand(session, priority),
  m_folderId(folderId),
  m_syncSessionReady(false),
  m_syncSessionReadySlot(this, &ImapSyncSessionCommand::SyncSessionReady)
{
}

ImapSyncSessionCommand::~ImapSyncSessionCommand()
{
}

bool ImapSyncSessionCommand::PrepareToRun()
{
	CommandTraceFunction();

	if(!ImapSessionCommand::PrepareToRun()) {
		// Parent class isn't ready to let us run yet
		return false;
	}

	if(m_syncSessionReady) {
		return true;
	}

	try {
		if(m_session.GetClient().get() == NULL) {
			throw MailException("can't run sync session command without a client", __FILE__, __LINE__);
		}

		m_session.GetClient()->RequestSyncSession(m_folderId, this, m_syncSessionReadySlot);

		// SyncSessionReady will be called when the sync session is ready to run
	} catch(const exception& e) {
		Failure(e);
	} catch(...) {
		Failure(UNKNOWN_EXCEPTION);
	}

	return false;
}

void ImapSyncSessionCommand::SetSyncSession(const MojRefCountedPtr<SyncSession>& syncSession)
{
	m_syncSession = syncSession;
}

MojErr ImapSyncSessionCommand::SyncSessionReady()
{
	CommandTraceFunction();

	m_syncSessionReady = true;
	RunIfReady();

	return MojErrNone;
}

MojInt64 ImapSyncSessionCommand::GetLastSyncRev() const
{
	if(m_syncSession.get() != NULL)
		return m_syncSession->GetLastSyncRev();
	else
		throw MailException("no sync session available", __FILE__, __LINE__);
}

void ImapSyncSessionCommand::Failure(const exception& e)
{
	if(m_syncSession.get() != NULL)
		m_syncSession->CommandFailed(this, e);

	ImapSessionCommand::Failure(e);
}

void ImapSyncSessionCommand::Complete()
{
	if(m_syncSession.get() != NULL)
		m_syncSession->CommandCompleted(this);

	ImapSessionCommand::Complete();
}

void ImapSyncSessionCommand::Cleanup()
{
	m_syncSessionReadySlot.cancel();

	ImapCommand::Cleanup();
}
