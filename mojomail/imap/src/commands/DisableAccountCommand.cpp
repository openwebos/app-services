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

#include "commands/DisableAccountCommand.h"
#include "ImapClient.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "data/FolderAdapter.h"
#include "ImapPrivate.h"
#include "commands/PurgeEmailsCommand.h"
#include "commands/ImapCommandResult.h"
#include "core/MojServiceMessage.h"
#include "exceptions/MojErrException.h"
#include "commands/DeleteActivitiesCommand.h"

DisableAccountCommand::DisableAccountCommand(ImapClient& client, const MojRefCountedPtr<MojServiceMessage>& msg, const MojObject& payload)
: ImapClientCommand(client),
  m_msg(msg),
  m_payload(payload),
  m_getFoldersSlot(this, &DisableAccountCommand::GetFoldersResponse),
  m_purgeEmailsSlot(this, &DisableAccountCommand::PurgeEmailsDone),
  m_deleteFoldersSlot(this, &DisableAccountCommand::DeleteFoldersResponse),
  m_deleteActivitiesSlot(this, &DisableAccountCommand::DeleteActivitiesDone),
  m_updateAccountSlot(this, &DisableAccountCommand::UpdateAccountResponse),
  m_smtpAccountDisabledSlot(this, &DisableAccountCommand::SmtpAccountDisabledResponse)
{
}

DisableAccountCommand::~DisableAccountCommand()
{
}

void DisableAccountCommand::RunImpl()
{
	CommandTraceFunction();

	GetFolders();
}

void DisableAccountCommand::GetFolders()
{
	CommandTraceFunction();

	m_client.GetDatabaseInterface().GetFolders(m_getFoldersSlot, m_client.GetAccountId(), m_folderPage, true);
}

MojErr DisableAccountCommand::GetFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		BOOST_FOREACH(const MojObject& folderObj, DatabaseAdapter::GetResultsIterators(response)) {
			MojObject folderId;
			err = folderObj.getRequired(FolderAdapter::ID, folderId);
			ErrorToException(err);

			m_folderIds.push(folderId);
		}

		// initialize iterator
		m_folderIdIterator = m_folderIds.begin();

		PurgeNextFolderEmails();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DisableAccountCommand::PurgeNextFolderEmails()
{
	CommandTraceFunction();

	if(m_folderIdIterator != m_folderIds.end()) {
		MojObject folderId = *m_folderIdIterator;
		++m_folderIdIterator;

		if(IsValidId(folderId))
		{
			m_purgeCommand.reset((new PurgeEmailsCommand(m_client, folderId)));
			m_purgeCommand->Run(m_purgeEmailsSlot);
		}
		else
		{
			PurgeEmailsDone();
		}
	} else {
		DeleteFolders();
	}

	// TODO: check for more results from database
}

MojErr DisableAccountCommand::PurgeEmailsDone()
{
	CommandTraceFunction();

	try {
		// Check for error
		m_purgeCommand->GetResult()->CheckException();

		PurgeNextFolderEmails();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DisableAccountCommand::DeleteFolders()
{
	CommandTraceFunction();

	m_client.GetDatabaseInterface().PurgeIds(m_deleteFoldersSlot, m_folderIds);
}

MojErr DisableAccountCommand::DeleteFoldersResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		// Delete watches and activities
		DeleteActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DisableAccountCommand::DeleteActivities()
{
	CommandTraceFunction();

	// Delete anything that includes the accountId in the name
	MojString accountIdSubstring;
	MojErr err = accountIdSubstring.format("accountId=%s", AsJsonString(m_client.GetAccountId()).c_str());
	ErrorToException(err);

	m_deleteActivitiesCommand.reset(new DeleteActivitiesCommand(m_client));
	m_deleteActivitiesCommand->SetIncludeNameFilter(accountIdSubstring);
	m_deleteActivitiesCommand->Run(m_deleteActivitiesSlot);
}

MojErr DisableAccountCommand::DeleteActivitiesDone()
{
	CommandTraceFunction();

	try {
		m_deleteActivitiesCommand->GetResult()->CheckException();

		UpdateAccount();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DisableAccountCommand::UpdateAccount()
{
	CommandTraceFunction();

	MojObject null(MojObject::TypeNull);

	// Use a blank account since the client may not have loaded the account
	ImapAccount account;

	account.SetAccountId( m_client.GetAccountId() );

	account.SetInboxFolderId(null);
	account.SetDraftsFolderId(null);
	account.SetSentFolderId(null);
	account.SetOutboxFolderId(null);
	account.SetTrashFolderId(null);

	m_client.GetDatabaseInterface().UpdateAccountSpecialFolders(m_updateAccountSlot, account);
}

MojErr DisableAccountCommand::UpdateAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		m_client.SendRequest(m_smtpAccountDisabledSlot, "com.palm.smtp", "accountEnabled", m_payload);
	} CATCH_AS_FAILURE
	
	return MojErrNone;
}

MojErr DisableAccountCommand::SmtpAccountDisabledResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		Done();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DisableAccountCommand::Done()
{
	m_client.DisabledAccount();

	if(m_msg.get()) {
		m_msg->replySuccess();
	}

	Complete();
}

void DisableAccountCommand::Failure(const std::exception& e)
{
	// TODO client needs to cleanup state after failure
	ImapClientCommand::Failure(e);

	m_msg->replyError(MojErrInternal, e.what());
}
