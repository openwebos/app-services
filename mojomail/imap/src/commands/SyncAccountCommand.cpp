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

#include "commands/SyncAccountCommand.h"
#include "data/ImapAccount.h"
#include "ImapClient.h"
#include "ImapPrivate.h"

SyncAccountCommand::SyncAccountCommand(ImapClient& client, SyncParams syncParams)
: ImapClientCommand(client),
  m_syncParams(syncParams)
{

}

SyncAccountCommand::~SyncAccountCommand()
{
}

void SyncAccountCommand::RunImpl()
{
	MojObject inboxFolderId = m_client.GetAccount().GetInboxFolderId();

	if(IsValidId(inboxFolderId)) {
		// Sync inbox
		m_client.SyncFolder(inboxFolderId, m_syncParams);
	} else {
		MojLogCritical(m_log, "error in SyncAccountCommand: no inboxFolderId");
	}

	Complete();
}
