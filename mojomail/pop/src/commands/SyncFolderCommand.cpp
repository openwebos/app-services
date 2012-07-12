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

#include "activity/Activity.h"
#include "activity/ActivityParser.h"
#include "client/PopSession.h"
#include "client/SyncSession.h"
#include "commands/SyncFolderCommand.h"
#include "PopDefs.h"

SyncFolderCommand::SyncFolderCommand(PopClient& client, const MojObject& payload)
: PopClientCommand(client, "Sync POP folder"),
  m_payload(payload),
  m_outboxSyncResponseSlot(this, &SyncFolderCommand::OutboxSyncResponse),
  m_syncSessionCompletedSlot(this, &SyncFolderCommand::SyncSessionCompletedResponse)
{
}

SyncFolderCommand::~SyncFolderCommand()
{

}

// TODO: switch to ActivitySet
void SyncFolderCommand::RunImpl()
{
	try {
		if (!m_client.GetAccount().get()) {
			MojString err;
			err.format("Account is not loaded for '%s'", AsJsonString(
					m_client.GetAccountId()).c_str());
			throw MailException(err.data(), __FILE__, __LINE__);
		}

		MojLogInfo(m_log, "Running sync folder command in PopClient with payload: %s", AsJsonString(m_payload).c_str());
		m_activity = ActivityParser::GetActivityFromPayload(m_payload);

		MojErr err;
		err = m_payload.getRequired("folderId", m_folderId);
		ErrorToException(err);

		bool exist = m_payload.get("force", m_force);
		if (!exist) {
			// just in case this value is not set by MojObject if "force" field doesn't exist
			m_force = false;
		}

		MojObject outboxId = m_client.GetAccount()->GetOutboxFolderId();
		if (outboxId == m_folderId) {
			m_client.SendRequest(m_outboxSyncResponseSlot, "com.palm.smtp",
					"syncOutbox", m_payload);
		} else {
			SyncFolder();
		}
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception", __FILE__, __LINE__));
	}
}

MojErr SyncFolderCommand::OutboxSyncResponse(MojObject& response, MojErr err) {
	try {
		ErrorToException(err);

		Complete();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void SyncFolderCommand::SyncFolder()
{
	PopClient::AccountPtr account = m_client.GetAccount();
	MojLogInfo(m_log, "inbox=%s, syncing folder=%s, force=%d",
				AsJsonString(account->GetInboxFolderId()).c_str(),
				AsJsonString(m_folderId).c_str(),
				m_force);
	if (m_folderId != account->GetInboxFolderId()) {
		// POP transport only supports inbox sync.
		MojLogInfo(m_log, "POP transport will skip non-inbox syncing");
		Complete();
		return;
	}

	SyncSessionPtr sSession = m_client.GetOrCreateSyncSession(m_folderId);
	m_activity = ActivityParser::GetActivityFromPayload(m_payload);
	if (!sSession->IsActive() && !sSession->IsEnding()) {
		// since sync session is not running, we can start the folder sync
		MojLogDebug(m_log, "Sync session is not active");
		StartFolderSync(m_activity);

		Complete();
	} else {
		if (m_force) {
			// Manual Sync and session is either active or ending

			// TODO: sync session is in progress, need to terminate the current
			// sync session first.  Then re-start sync session.
			StartFolderSync(m_activity);

			Complete();
		} else {
			// This is either auto sync, schedule sync, retry sync.
			if (sSession->IsActive()) {

				if (m_activity.get()) {
					MojLogInfo(m_log, "Attach sync activity to actively syncing session");

					// Adopt activity and then add it to sync session
					m_activity->Adopt(m_client);
					sSession->AttachActivity(m_activity);
				}

				Complete();
			} else if (sSession->IsEnding()) {
				// wait for sync session to end and then start folder sync
				MojLogInfo(m_log, "Waiting for ending sync session to complete");
				sSession->WaitForSessionComplete(m_syncSessionCompletedSlot);
			}
		}
	}
}

void SyncFolderCommand::StartFolderSync(ActivityPtr activity)
{
	PopClient::SyncSessionPtr sSession = m_client.GetSyncSession();
	PopClient::PopSessionPtr pSession = m_client.GetSession();

	// adopt the activity before syncing folder
	if (m_activity.get()) {
		sSession->AdoptActivity(activity);
	}

	MojLogInfo(m_log, "Starting folder sync from POP session");
	pSession->SyncFolder(m_folderId, m_force);
}

MojErr SyncFolderCommand::SyncSessionCompletedResponse() {
	try {
		// start another sync after previous sync completes
		StartFolderSync(m_activity);

		Complete();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}
