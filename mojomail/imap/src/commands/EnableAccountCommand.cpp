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

#include "commands/EnableAccountCommand.h"
#include "commands/CreateDefaultFoldersCommand.h"
#include "ImapClient.h"
#include "ImapPrivate.h"

EnableAccountCommand::EnableAccountCommand(ImapClient& client, MojServiceMessage* msg)
: ImapClientCommand(client),
  m_msg(msg),
  m_capabilityProvider("com.palm.imap.mail"),
  m_busAddress("com.palm.imap"),
  m_createDefaultFoldersSlot(this, &EnableAccountCommand::CreateDefaultFoldersDone),
  m_enableSmtpAccountSlot(this, &EnableAccountCommand::EnableSmtpAccountResponse),
  m_updateAccountSlot(this, &EnableAccountCommand::UpdateAccountDone),
  m_updateMissingCredentialsResponseSlot(this, &EnableAccountCommand::UpdateMissingCredentialsSyncStatusResponse)
{
}

EnableAccountCommand::~EnableAccountCommand()
{
}

void EnableAccountCommand::RunImpl()
{
	CreateDefaultFolders();
}

void EnableAccountCommand::CreateDefaultFolders()
{
	CommandTraceFunction();

	m_createDefaultFoldersCommand.reset(new CreateDefaultFoldersCommand(m_client));
	m_createDefaultFoldersCommand->Run(m_createDefaultFoldersSlot);
}

MojErr EnableAccountCommand::CreateDefaultFoldersDone()
{
	CommandTraceFunction();

	try {
		m_createDefaultFoldersCommand->GetResult()->CheckException();

		EnableSmtpAccount();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void EnableAccountCommand::EnableSmtpAccount()
{
	CommandTraceFunction();

	MojObject payload;
	MojErr err;
	err = payload.put("accountId", m_client.GetAccountId());
	ErrorToException(err);
	err = payload.put("enabled", true);
	ErrorToException(err);

	m_client.SendRequest(m_enableSmtpAccountSlot, "com.palm.smtp", "accountEnabled", payload);
}

MojErr EnableAccountCommand::EnableSmtpAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);
		UpdateMissingCredentialsSyncStatus();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void EnableAccountCommand::UpdateMissingCredentialsSyncStatus()
{
	CommandTraceFunction();

	if(!m_client.GetAccount().HasPassword())
	{
		fprintf(stderr, "EnableAccountCommand::UpdateMissingCredentialsSyncStatus(): no password!\n");
		MojObject accountId = m_client.GetAccountId();
		SyncState syncState(accountId);
		syncState.SetState(SyncState::ERROR);
		syncState.SetError("CREDENTIALS_NOT_FOUND", "error: credentials not found!");
		m_syncStateUpdater.reset(new SyncStateUpdater(m_client, m_capabilityProvider.data(), m_busAddress.data()));
		m_syncStateUpdater->UpdateSyncState(m_updateMissingCredentialsResponseSlot, syncState);
	} else {
		fprintf(stderr, "EnableAccountCommand::UpdateMissingCredentialsSyncStatus(): has password!\n");
		UpdateAccount();
	}
}

MojErr EnableAccountCommand::UpdateMissingCredentialsSyncStatusResponse()
{
	fprintf(stderr, "EnableAccountCommand::UpdateMissingCredentialsSyncStatusResponse(): sync status updated\n");
	Done();
	return MojErrNone;
}

void EnableAccountCommand::UpdateAccount()
{
	CommandTraceFunction();

	m_updateAccountCommand.reset(new UpdateAccountCommand(m_client, NULL, false));
	m_updateAccountCommand->Run(m_updateAccountSlot);
}

MojErr EnableAccountCommand::UpdateAccountDone()
{
	CommandTraceFunction();

	try {
		m_updateAccountCommand->GetResult()->CheckException();
		Done();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void EnableAccountCommand::Done()
{
	CommandTraceFunction();

	MojLogNotice(m_log, "enabled account %s successfully", m_client.GetAccountIdString().c_str());

	if (m_msg.get()) {
		m_msg->replySuccess();
		m_msg.reset();
	}

	Complete();
}

void EnableAccountCommand::Failure(const exception& e)
{
	MojLogError(m_log, "error enabling account %s: %s", m_client.GetAccountIdString().c_str(), e.what());

	if (m_msg.get()) {
		m_msg->replyError(MojErrInternal, e.what());
		m_msg.reset();
	}

	ImapClientCommand::Failure(e);
}

bool EnableAccountCommand::Cancel(CancelType cancelType)
{
	if (m_msg.get()) {
		m_msg->replyError(MojErrNotFound, "failed to find account");
		m_msg.reset();
	}

	return ImapClientCommand::Cancel(cancelType);
}

void EnableAccountCommand::Status(MojObject& status) const
{
	MojErr err;
	ImapClientCommand::Status(status);

	if(m_createDefaultFoldersCommand.get()) {
		MojObject commandStatus;
		m_createDefaultFoldersCommand->Status(commandStatus);

		err = status.put("createDefaultFoldersCommand", commandStatus);
		ErrorToException(err);
	}

	if(m_updateAccountCommand.get()) {
		MojObject commandStatus;
		m_updateAccountCommand->Status(commandStatus);

		err = status.put("updateAccountCommand", commandStatus);
		ErrorToException(err);
	}
}
