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

#include "client/MockImapClient.h"
#include "data/MockDatabase.h"
#include <boost/shared_ptr.hpp>
#include "client/MockSyncSession.h"

static MojObject s_fakeAccountId;

MockImapClient::MockImapClient(const boost::shared_ptr<MockDatabase>& db)
: ImapClient(NULL, NULL, s_fakeAccountId, db)
{
	m_account.reset(new ImapAccount());
	m_account->SetAccountId(s_fakeAccountId);
}

MockImapClient::~MockImapClient()
{
}

void MockImapClient::SendRequest(MojServiceRequest::ReplySignal::SlotRef handler, const MojChar* service, const MojChar* method,
		const MojObject& payload, MojUInt32 numReplies)
{
	// FIXME
	throw MailException("MockImapClient::SendRequest not implemented", __FILE__, __LINE__);
}

void MockImapClient::RequestSyncSession(const MojObject& folderId, ImapSyncSessionCommand* command, MojSignal<>::SlotRef slot)
{
	ImapClient::RequestSyncSession(folderId, command, slot);
}

MockImapClient::SyncSessionPtr MockImapClient::GetOrCreateSyncSession(const MojObject& folderId)
{
	MojRefCountedPtr<SyncSession>& syncSession = m_syncSessions[folderId];

	if(syncSession.get() == NULL) {
		syncSession.reset(new MockSyncSession(*this, folderId));
	}

	return syncSession;
}

void MockImapClient::CheckQueue()
{
}
