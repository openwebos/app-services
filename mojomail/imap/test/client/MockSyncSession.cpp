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

#include "client/MockSyncSession.h"
#include "data/ImapFolder.h"
#include "data/DatabaseAdapter.h"

MockSyncSession::MockSyncSession(MockImapClient& client, const MojObject& folderId)
: SyncSession(client, folderId)
{
	// FIXME
	m_state = State_Active;
}

MockSyncSession::~MockSyncSession()
{
}

void MockSyncSession::RegisterCommand(Command* command, MojSignal<>::SlotRef readySlot)
{
	MojSignal<> signal(this);
	signal.connect(readySlot);

	signal.fire();
}

void MockSyncSession::CommandCompleted(Command* command)
{
}

