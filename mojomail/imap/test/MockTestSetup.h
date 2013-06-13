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

#ifndef MOCKTESTSETUP_H_
#define MOCKTESTSETUP_H_

#include "client/MockImapClient.h"
#include "client/MockImapSession.h"
#include "data/MockDatabase.h"
#include <boost/make_shared.hpp>

class MockTestSetup
{
public:
	MockTestSetup();
	virtual ~MockTestSetup();

	template<typename DATABASE>
	DATABASE& GetTestDatabase()
	{
		if(!m_database.get()) {
			m_database = boost::make_shared<DATABASE>();
		}
		return dynamic_cast<DATABASE&>(*m_database);
	}

	void SetDatabase(const boost::shared_ptr<MockDatabase>& db) { m_database = db; }

	boost::shared_ptr<MockDatabase>&	GetDatabasePtr();
	MockDatabase&						GetDatabase() { return *GetDatabasePtr(); }

	MojRefCountedPtr<MockImapClient>&	GetClientPtr();
	MockImapClient&						GetClient() { return *GetClientPtr(); }
	MockImapSession&					GetSession();

protected:
	boost::shared_ptr<ImapAccount>		m_account;
	boost::shared_ptr<MockDatabase>		m_database;
	MojRefCountedPtr<MockImapClient>	m_client;
	MojRefCountedPtr<MockImapSession>	m_session;
};

#endif /* MOCKTESTSETUP_H_ */
