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

#include "MockTestSetup.h"

MockTestSetup::MockTestSetup()
{
	m_account.reset(new ImapAccount());
}

MockTestSetup::~MockTestSetup()
{
}

boost::shared_ptr<MockDatabase>& MockTestSetup::GetDatabasePtr()
{
	if(!m_database.get()) {
		m_database.reset(new MockDatabase());
	}

	return m_database;
}

MojRefCountedPtr<MockImapClient>& MockTestSetup::GetClientPtr()
{
	if(!m_client.get()) {
		m_client.reset(new MockImapClient(GetDatabasePtr()));
		m_client->SetAccount(m_account);
	}

	return m_client;
}

MockImapSession& MockTestSetup::GetSession()
{
	if(!m_session.get()) {
		m_session.reset(new MockImapSession(GetClientPtr(), GetDatabase()));
		m_session->SetBusClient(*GetClientPtr());
		m_session->SetAccount(m_account);
	}

	return *m_session;
}
