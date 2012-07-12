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

#include "client/ImapSession.h"
#include "protocol/ExamineResponseParser.h"
#include "client/FolderSession.h"
#include "ImapPrivate.h"
#include "data/ImapFolderAdapter.h"
#include "commands/SelectFolderCommand.h"
#include "data/DatabaseInterface.h"
#include "data/DatabaseAdapter.h"
#include "data/FolderAdapter.h"
#include "commands/PurgeEmailsCommand.h"

SelectFolderCommand::SelectFolderCommand(ImapSession& session, const MojObject& folderId)
: ImapSessionCommand(session),
  m_selectFolderId(folderId),
  m_initialSync(false),
  m_getFolderResponseSlot(this, &SelectFolderCommand::GetFolderResponse),
  m_selectFolderResponseSlot(this, &SelectFolderCommand::SelectResponse),
  m_purgeEmailsSlot(this, &SelectFolderCommand::PurgeEmailsDone),
  m_updateFolderSlot(this, &SelectFolderCommand::UpdateFolderResponse)
{
}

SelectFolderCommand::~SelectFolderCommand()
{
}

void SelectFolderCommand::RunImpl()
{
	GetFolder();
}

void SelectFolderCommand::GetFolder()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().GetById(m_getFolderResponseSlot, m_selectFolderId);
}

MojErr SelectFolderCommand::GetFolderResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// Check database response
		ErrorToException(err);

		MojObject folderObj;
		DatabaseAdapter::GetOneResult(response, folderObj);

		m_folder.reset(new ImapFolder());
		ImapFolderAdapter::ParseDatabaseObject(folderObj, *m_folder.get());

		MojObject lastSyncRev;
		if(folderObj.get(ImapFolderAdapter::LAST_SYNC_REV, lastSyncRev) && !lastSyncRev.null()) {
			m_initialSync = false;
		} else {
			m_initialSync = true;
		}

		// Migrate from older folders without a uid validity
		if(!folderObj.contains(ImapFolderAdapter::UIDVALIDITY)) {
			m_initialSync = true;
		}

		SelectFolder();

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SelectFolderCommand::SelectFolder()
{
	CommandTraceFunction();

	m_responseParser.reset(new ExamineResponseParser(m_session, m_selectFolderResponseSlot));
	m_session.SendRequest("SELECT " + QuoteString(m_folder->GetFolderName()), m_responseParser);
}

MojErr SelectFolderCommand::SelectResponse()
{
	CommandTraceFunction();

	try {
		ImapStatusCode status = m_responseParser->GetStatus();
		if(status == OK) {
			UID serverUIDValidity = m_responseParser->GetUIDValidity();
			UID localUIDValidity = m_folder->GetUIDValidity();

			MojLogDebug(m_log, "UIDValidity: local=%d, server=%d", localUIDValidity, serverUIDValidity);

			if(localUIDValidity != serverUIDValidity && serverUIDValidity > 0) {
				if(m_initialSync && localUIDValidity == 0) {
					// If we've never sync'd this folder before, just save the UID validity
					UpdateFolder();
				} else {
					// If the UID validity changed, we need to remove all emails from the folder
					MojLogWarning(m_log, "folder %s uid validity changed: %d local, %d on server", AsJsonString(m_selectFolderId).c_str(), localUIDValidity, serverUIDValidity);
					PurgeEmails();
				}
			} else {
				// Same UIDValidity
				SelectDone();
			}
		} else {
			// FIXME Check whether failure was due to bad server response or connection failure
			// FIXME need client to refresh folder list
			m_responseParser->CheckStatus();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

// UID validity didn't match; need to remove the folder's existing contents
void SelectFolderCommand::PurgeEmails()
{
	m_purgeEmailsCommand.reset(new PurgeEmailsCommand(*m_session.GetClient(), m_selectFolderId));
	m_purgeEmailsCommand->Run(m_purgeEmailsSlot);
}

MojErr SelectFolderCommand::PurgeEmailsDone()
{
	CommandTraceFunction();

	try {
		UpdateFolder();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SelectFolderCommand::UpdateFolder()
{
	CommandTraceFunction();

	MojErr err;
	MojObject updatedFolder;

	err = updatedFolder.put(DatabaseAdapter::ID, m_folder->GetId());
	ErrorToException(err);

	// Update UIDVALIDITY
	m_folder->SetUIDValidity(m_responseParser->GetUIDValidity());
	err = updatedFolder.put(ImapFolderAdapter::UIDVALIDITY, (MojInt64) m_folder->GetUIDValidity());
	ErrorToException(err);

	m_session.GetDatabaseInterface().UpdateFolder(m_updateFolderSlot, updatedFolder);
}

MojErr SelectFolderCommand::UpdateFolderResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		SelectDone();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SelectFolderCommand::SelectDone()
{
	CommandTraceFunction();

	boost::shared_ptr<FolderSession> folderSession = make_shared<FolderSession>(m_folder);

	if(m_responseParser->GetExistsCount() > 0) // -1 means unknown
		folderSession->SetMessageCount( m_responseParser->GetExistsCount() );

	m_session.SetFolderSession(folderSession);
	m_session.FolderSelected();
	Complete();
}

void SelectFolderCommand::Failure(const exception& e)
{
	ImapSessionCommand::Failure(e);
	m_session.FolderSelectFailed();
}

std::string SelectFolderCommand::Describe() const
{
	return ImapSessionCommand::Describe(m_selectFolderId);
}
