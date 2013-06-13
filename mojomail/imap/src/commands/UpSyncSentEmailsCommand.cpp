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

#include "commands/UpSyncSentEmailsCommand.h"
#include "commands/ImapCommandResult.h"
#include "client/ImapSession.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "data/EmailSchema.h"
#include "data/EmailAdapter.h"
#include "data/ImapFolderAdapter.h"
#include "ImapPrivate.h"
#include "activity/ImapActivityFactory.h"
#include "activity/ActivityBuilder.h"
#include "activity/ActivitySet.h"

// FIXME low value for testing
const int UpSyncSentEmailsCommand::LOCAL_BATCH_SIZE = 10;

UpSyncSentEmailsCommand::UpSyncSentEmailsCommand(ImapSession& session, const MojObject& folderId)
: ImapSyncSessionCommand(session, folderId),
  m_getSentFolderNameSlot(this, &UpSyncSentEmailsCommand::GetSentFolderNameResponse),
  m_getSentEmailsSlot(this, &UpSyncSentEmailsCommand::GetSentEmailsResponse),
  m_appendResponseSlot(this, &UpSyncSentEmailsCommand::AppendCommandResponse),
  m_purgeSentEmailSlot(this, &UpSyncSentEmailsCommand::PurgeSentEmailResponse),
  m_replaceActivitySlot(this, &UpSyncSentEmailsCommand::ReplaceActivityDone)
{
}

UpSyncSentEmailsCommand::~UpSyncSentEmailsCommand()
{
}

void UpSyncSentEmailsCommand::RunImpl()
{
	GetSentFolderName();
}

void UpSyncSentEmailsCommand::GetSentFolderName()
{
	CommandTraceFunction();
	m_session.GetDatabaseInterface().GetFolderName(m_getSentFolderNameSlot, m_folderId);
}

MojErr UpSyncSentEmailsCommand::GetSentFolderNameResponse(MojObject& response, MojErr err)
{
	try {
			ErrorToException(err);

			MojObject folderObj;
			DatabaseAdapter::GetOneResult(response, folderObj);

			MojString sentFolderName;
			err = folderObj.getRequired(ImapFolderAdapter::SERVER_FOLDER_NAME, sentFolderName);
			ErrorToException(err);

			m_sentFolderName.assign(sentFolderName);

			GetSentEmails();

		} CATCH_AS_FAILURE

		return MojErrNone;
}

void UpSyncSentEmailsCommand::GetSentEmails()
{
	CommandTraceFunction();

	MojObject outboxId = m_session.GetAccount()->GetOutboxFolderId();
	m_session.GetDatabaseInterface().GetSentEmails(m_getSentEmailsSlot, outboxId, m_sentEmailsPage, LOCAL_BATCH_SIZE);
}

MojErr UpSyncSentEmailsCommand::GetSentEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		// Store all sent emails in a vector.
		BOOST_FOREACH(const MojObject& emailObj, DatabaseAdapter::GetResultsIterators(response))
		{
			EmailPtr email = boost::make_shared<Email>();
			EmailAdapter::ParseDatabaseObject(emailObj, *email);
			MojObject id;
			err = emailObj.getRequired(DatabaseAdapter::ID, id);
			ErrorToException(err);
			email->SetId(id);
			m_sentEmails.push_back(email);
		}

		// Check for more results
		bool hasMoreResults = DatabaseAdapter::GetNextPage(response, m_sentEmailsPage);

		if(hasMoreResults) {
			GetSentEmails();
		} else {
			if(!m_sentEmails.empty()) {
				MojLogInfo(m_log, "up sync %d sent emails to the server", m_sentEmails.size());
				m_emailP = m_sentEmails.back();
				CreateAndRunAppendCommand();
			} else {
				UpSyncSentItemsComplete();
			}
		}

	} CATCH_AS_FAILURE

	return MojErrNone;
}

MojErr UpSyncSentEmailsCommand::AppendCommandResponse()
{

	MojLogInfo(m_log, "IMAP APPEND command response");

	try
	{
		m_appendResult->CheckException();
		PurgeSentEmail();
	}
	CATCH_AS_FAILURE

	return MojErrNone;
}


void UpSyncSentEmailsCommand::PurgeSentEmail()
{
	MojErr err;

	// Delete and purge from device
	MojObject id = m_emailP->GetId();

	MojLogInfo(m_log, "deleting uploaded sent email %s", AsJsonString(id).c_str());

	err = m_pendingPurge.push(id);
	ErrorToException(err);

	m_session.GetDatabaseInterface().DeleteEmailIds(m_purgeSentEmailSlot, m_pendingPurge);
}

MojErr UpSyncSentEmailsCommand::PurgeSentEmailResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);
		m_sentEmails.pop_back();

		// Append the next sent email if there is more in the queue.
		if(!m_sentEmails.empty())
		{
			m_emailP = m_sentEmails.back();
			CreateAndRunAppendCommand();
		}
		else
		{
			UpSyncSentItemsComplete();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void UpSyncSentEmailsCommand::CreateAndRunAppendCommand()
{
	m_appendCommand.reset((new AppendCommand(m_session, m_folderId, m_emailP, "\\Seen", m_sentFolderName)));
	m_appendResult.reset(new ImapCommandResult(m_appendResponseSlot));
	m_appendCommand->SetResult(m_appendResult);
	m_appendCommand->Run();
}

void UpSyncSentEmailsCommand::UpSyncSentItemsComplete()
{
	MojObject accountId = m_session.GetAccount()->GetId();
	MojObject folderId = m_session.GetAccount()->GetOutboxFolderId();

	ImapActivityFactory factory;
	ActivityBuilder ab;

	factory.BuildOutboxWatch(ab, accountId, folderId, 0);

	m_commandActivitySet->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
	m_commandActivitySet->EndActivities(m_replaceActivitySlot);
}

MojErr UpSyncSentEmailsCommand::ReplaceActivityDone()
{
	CommandTraceFunction();

	try {
		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}
