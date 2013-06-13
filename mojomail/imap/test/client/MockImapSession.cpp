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

#include "client/MockImapSession.h"
#include "data/MockDatabase.h"
#include "stream/MockStreams.h"
#include "client/MockImapClient.h"

MockImapSession::MockImapSession(const MojRefCountedPtr<MockImapClient>& client)
: ImapSession(client.get())
{
	Init();
	SetDatabase(m_defaultMockDatabase);
}

MockImapSession::MockImapSession(const MojRefCountedPtr<MockImapClient>& client, MockDatabase& database)
: ImapSession(client.get())
{
	Init();
	SetDatabase(database);
}

MockImapSession::~MockImapSession()
{
}

void MockImapSession::Init()
{
	m_mockInputStream.reset(new MockInputStream());
	m_mockOutputStream.reset(new MockOutputStream());
	m_inputStream = m_mockInputStream;
	m_outputStream = m_mockOutputStream;

	m_client->GetAccount();
}

void MockImapSession::CheckQueue()
{
}
