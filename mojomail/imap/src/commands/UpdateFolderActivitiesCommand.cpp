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

#include "commands/UpdateFolderActivitiesCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "ImapClient.h"
#include "ImapPrivate.h"
#include "activity/ActivitySet.h"
#include "activity/ActivityBuilder.h"
#include "activity/ImapActivityFactory.h"
#include "commands/DeleteActivitiesCommand.h"

UpdateFolderActivitiesCommand::UpdateFolderActivitiesCommand(ImapClient& client, const MojObject::ObjectVec& folderIds)
: ImapClientCommand(client),
  m_folderIds(folderIds),
  m_activitySet(new ActivitySet(client)),
  m_getSomeFoldersSlot(this, &UpdateFolderActivitiesCommand::GetSomeFoldersResponse),
  m_getAllFoldersSlot(this, &UpdateFolderActivitiesCommand::GetAllFoldersResponse),
  m_deleteActivitiesSlot(this, &UpdateFolderActivitiesCommand::ClearFolderActivitiesDone),
  m_updatedSlot(this, &UpdateFolderActivitiesCommand::ActivitiesUpdated)
{
}

UpdateFolderActivitiesCommand::~UpdateFolderActivitiesCommand()
{
}

void UpdateFolderActivitiesCommand::RunImpl()
{
	GetAllFolders();
}

void UpdateFolderActivitiesCommand::GetSomeFolders()
{
}

MojErr UpdateFolderActivitiesCommand::GetSomeFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		BOOST_FOREACH(const MojObject& obj, DatabaseAdapter::GetResultsIterators(response)) {
			m_folders.push_back(obj);
		}

		UpdateActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void UpdateFolderActivitiesCommand::GetAllFolders()
{
	CommandTraceFunction();

	m_client.GetDatabaseInterface().GetFolders(m_getAllFoldersSlot, m_client.GetAccountId(), m_folderPage, false);
}

MojErr UpdateFolderActivitiesCommand::GetAllFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		BOOST_FOREACH(const MojObject& obj, DatabaseAdapter::GetResultsIterators(response)) {
			m_folders.push_back(obj);
		}

		if(DatabaseAdapter::GetNextPage(response, m_folderPage)) {
			GetAllFolders();
		} else {
			if(m_client.GetAccount().IsManualSync()) {
				ClearFolderActivities();
			} else {
				UpdateActivities();
			}
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void UpdateFolderActivitiesCommand::ClearFolderActivities()
{
	CommandTraceFunction();

	MojErr err;

	if(!m_folders.empty()) {
		MojObject folder = m_folders.back();
		m_folders.pop_back();

		MojObject folderId;
		err = folder.getRequired(DatabaseAdapter::ID, folderId);
		ErrorToException(err);

		m_deleteActivitiesCommand.reset(new DeleteActivitiesCommand(m_client));

		MojString folderIdJson;
		err = folderId.toJson(folderIdJson);
		ErrorToException(err);

		m_deleteActivitiesCommand->SetIncludeNameFilter(folderIdJson);
		m_deleteActivitiesCommand->Run(m_deleteActivitiesSlot);
	} else {
		Complete();
	}
}

MojErr UpdateFolderActivitiesCommand::ClearFolderActivitiesDone()
{
	CommandTraceFunction();

	try {
		// ignore errors

		ClearFolderActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

// TODO: make sure that this doesn't run while folders are being deleted
void UpdateFolderActivitiesCommand::UpdateActivities()
{
	CommandTraceFunction();

	MojErr err;
	ImapActivityFactory factory;

	BOOST_FOREACH(const MojObject& folder, m_folders) {
		MojObject folderId;
		err = folder.getRequired(DatabaseAdapter::ID, folderId);
		ErrorToException(err);

		if(!IsValidId(folderId)) {
			throw MailException("bad folder id", __FILE__, __LINE__);
		}

		//MojLogDebug(m_log, "updating activities for folder %s", AsJsonString(folderId).c_str());

		bool hasLastSyncRev;
		MojInt64 lastSyncRev = 0;
		hasLastSyncRev = folder.get("lastSyncRev", lastSyncRev);

		// Inbox activities
		if(folderId == m_client.GetAccount().GetInboxFolderId()) {
			int syncFrequency = m_client.GetAccount().GetSyncFrequencyMins();

			// Scheduled sync
			if(syncFrequency > 0) {
				// Create scheduled sync
				ActivityBuilder ab;

				int syncFrequencyMins = std::max(syncFrequency * 60, 5 * 60);
				factory.BuildScheduledSync(ab, m_client.GetAccountId(), folderId, syncFrequencyMins, true);

				m_activitySet->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
			} else {
				m_activitySet->CancelActivity(factory.GetScheduledSyncName(m_client.GetAccountId(), folderId));
			}

			// Push
			if(m_client.GetAccount().IsPush()) {
				// Create activity to keep idle alive across reboots
				ActivityBuilder ab;

				factory.BuildStartIdle(ab, m_client.GetAccountId(), folderId);
				m_activitySet->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
			} else {
				m_activitySet->CancelActivity(factory.GetStartIdleName(m_client.GetAccountId(), folderId));
			}
		}

		// Create watch activity if the folder has a lastSyncRev
		if(hasLastSyncRev && lastSyncRev > 0) {
			ActivityBuilder ab;
			factory.BuildFolderWatch(ab, m_client.GetAccountId(), folderId, lastSyncRev);

			m_activitySet->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
		}

		// Create draft watch activity for the drafts folder
		if(folderId == m_client.GetAccount().GetDraftsFolderId()) {
			ActivityBuilder ab;
			factory.BuildDraftsWatch(ab, m_client.GetAccountId(), folderId, lastSyncRev);

			m_activitySet->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
		}
	}

	m_activitySet->EndActivities(m_updatedSlot);
}

MojErr UpdateFolderActivitiesCommand::ActivitiesUpdated()
{
	Complete();

	return MojErrNone;
}

void UpdateFolderActivitiesCommand::Status(MojObject& status) const
{
	MojErr err;

	ImapClientCommand::Status(status);

	if(m_deleteActivitiesCommand.get()) {
		MojObject deleteStatus;
		m_deleteActivitiesCommand->Status(deleteStatus);
		err = status.put("m_deleteActivitiesCommand", deleteStatus);
		ErrorToException(err);
	}

	MojObject activityStatus;
	m_activitySet->Status(activityStatus);
	err = status.put("activitySet", activityStatus);
	ErrorToException(err);
}
