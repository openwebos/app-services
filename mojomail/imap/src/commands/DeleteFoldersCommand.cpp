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

#include "commands/DeleteFoldersCommand.h"
#include "commands/DeleteActivitiesCommand.h"
#include "commands/PurgeEmailsCommand.h"
#include "ImapPrivate.h"
#include "ImapClient.h"

DeleteFoldersCommand::DeleteFoldersCommand(ImapClient& client, const MojObject::ObjectVec& folderIds)
: ImapClientCommand(client),
  m_folderIds(folderIds),
  m_foldersToPurge(m_folderIds), // copy of list
  m_deleteActivitiesSlot(this, &DeleteFoldersCommand::DeleteActivitiesDone),
  m_purgeSlot(this, &DeleteFoldersCommand::PurgeEmailsDone),
  m_deleteFoldersSlot(this, &DeleteFoldersCommand::DeleteFoldersResponse)
{
}

DeleteFoldersCommand::~DeleteFoldersCommand()
{
}

void DeleteFoldersCommand::RunImpl()
{
	PurgeEmails();
}

void DeleteFoldersCommand::DeleteActivities()
{
	// TODO
	//m_deleteActivitiesCommand.reset(new DeleteActivitiesCommand(m_client));
}

MojErr DeleteFoldersCommand::DeleteActivitiesDone()
{
	// TODO
	return MojErrNone;
}

void DeleteFoldersCommand::PurgeEmails()
{
	CommandTraceFunction();

	if(!m_foldersToPurge.empty()) {
		MojObject folderId = m_foldersToPurge.back();
		m_foldersToPurge.pop();

		m_purgeEmailsCommand.reset(new PurgeEmailsCommand(m_client, folderId));
		m_purgeEmailsCommand->Run(m_purgeSlot);
	} else {
		DeleteFolders();
	}
}

MojErr DeleteFoldersCommand::PurgeEmailsDone()
{
	CommandTraceFunction();

	try {
		m_purgeEmailsCommand->GetResult()->CheckException();

		PurgeEmails();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DeleteFoldersCommand::DeleteFolders()
{
	CommandTraceFunction();

	m_client.GetDatabaseInterface().PurgeIds(m_deleteFoldersSlot, m_folderIds);
}

MojErr DeleteFoldersCommand::DeleteFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}
