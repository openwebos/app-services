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

#include "commands/SyncAccountCommand.h"
#include "data/PopAccountAdapter.h"
#include "data/PopFolderAdapter.h"
#include "PopDefs.h"

SyncAccountCommand::SyncAccountCommand(PopClient& client, MojObject& payload)
: PopClientCommand(client, "Sync POP account"),
  m_payload(payload)
{
}

SyncAccountCommand::~SyncAccountCommand()
{

}

void SyncAccountCommand::RunImpl()
{
	try {
		if (!m_client.GetAccount().get()) {
			MojString err;
			err.format("Account is not loaded for '%s'", AsJsonString(
					m_client.GetAccountId()).c_str());
			throw MailException(err.data(), __FILE__, __LINE__);
		}

		m_account = m_client.GetAccount();
		if(m_account->HasPassword()) {
			m_accountId = m_account->GetAccountId();
			MojObject inboxFolderId = m_account->GetInboxFolderId();

			MojLogInfo(m_log, "Creating command to sync inbox emails");

			MojErr err = m_payload.put(EmailSchema::FOLDER_ID, inboxFolderId);
			ErrorToException(err);

			m_client.SyncFolder(m_payload);
		} else {
			MojLogInfo(m_log, "No password!  Sync aborted!");
		}

		Complete();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception", __FILE__, __LINE__));
	}
}
