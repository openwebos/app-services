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

#ifndef MOCKIMAPCLIENT_H_
#define MOCKIMAPCLIENT_H_

#include "ImapClient.h"

class MockDatabase;
class MockSyncSession;

class MockImapClient : public ImapClient
{
	typedef MojRefCountedPtr<SyncSession> SyncSessionPtr;
public:
	MockImapClient(const boost::shared_ptr<MockDatabase>& db);
	virtual ~MockImapClient();

	// Overrides BusClient
	virtual void SendRequest(MojServiceRequest::ReplySignal::SlotRef handler, const MojChar* service, const MojChar* method,
			const MojObject& payload, MojUInt32 numReplies = 1);

	virtual void RequestSyncSession(const MojObject& folderId, ImapSyncSessionCommand* command, MojSignal<>::SlotRef slot);

	virtual SyncSessionPtr GetOrCreateSyncSession(const MojObject& folderId);

	virtual void CheckQueue();

	// NOTE: this does not override ImapClient
	void SetAccount(const boost::shared_ptr<ImapAccount>& account) { m_account = account; }
};

#endif /* MOCKIMAPCLIENT_H_ */
