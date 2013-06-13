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

#ifndef MOCKDATABASE_H_
#define MOCKDATABASE_H_

#include "data/DatabaseInterface.h"
#include "exceptions/MailException.h"
#include "db/MojDbClient.h"
#include <utility>

class ImapAccount;

class MockDatabase : public DatabaseInterface
{
public:
	MockDatabase() {}
	virtual ~MockDatabase() {}

#define DEFAULT Default(__func__, slot);

	virtual void Find(Signal::SlotRef slot, MojDbQuery query) { DEFAULT }
	virtual void GetById(Signal::SlotRef slot, const MojObject& id) { DEFAULT }
	virtual void GetByIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids) { DEFAULT }
	virtual void GetEmail(Signal::SlotRef slot, const MojObject& folderId, UID uid) { DEFAULT }

	virtual void CreateFolders(Signal::SlotRef slot, const MojObject::ObjectVec& array) { DEFAULT }
	virtual void ReserveIds(Signal::SlotRef slot, MojUInt32 num) { DEFAULT }
	virtual void DeleteIds(Signal::SlotRef slot, const MojObject::ObjectVec& array) { DEFAULT }
	virtual void PurgeIds(Signal::SlotRef slot, const MojObject::ObjectVec& array) { DEFAULT }
	virtual void DeleteFolderEmails(Signal::SlotRef slot, const MojObject& folderId) { DEFAULT }
	virtual void DeleteAccount(Signal::SlotRef slot, const MojObject& accountId) { DEFAULT }

	virtual void GetAccount(Signal::SlotRef slot, const MojObject& accountId) { DEFAULT }

	virtual void GetEmailChanges(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0) { DEFAULT }
	virtual void GetSentEmails(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0) { DEFAULT }
	virtual void GetDrafts(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0) { DEFAULT }
	virtual void GetMovedEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0) { DEFAULT }
	virtual void GetDeletedEmails(Signal::SlotRef slot, const MojObject& folderId, MojInt64 rev, const MojDbQuery::Page& page, MojInt32 limit = 0) { DEFAULT }
	virtual void GetEmailSyncList(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit = 0) { DEFAULT }
	
	virtual void GetAutoDownloads(Signal::SlotRef slot, const MojObject& folderId, const MojDbQuery::Page& page, MojInt32 limit) { DEFAULT }

	virtual void GetFolders(Signal::SlotRef slot, const MojObject& accountId, const MojDbQuery::Page& page, bool allFolders = false) { DEFAULT }
	virtual void GetFolderName(Signal::SlotRef slot, const MojObject& folderId) { DEFAULT }

	virtual void UpdateAccount(Signal::SlotRef slot, const MojObject& accountId, const MojObject& props) { DEFAULT }
	virtual void UpdateAccountSpecialFolders(Signal::SlotRef slot, const ImapAccount& account) { DEFAULT }
	virtual void UpdateAccountError(Signal::SlotRef slot, const MojObject& accountId, const MojObject& errStatus) { DEFAULT }
	virtual void UpdateAccountRetry(Signal::SlotRef slot, const MojObject& accountId, const MojObject& retryStatus) { DEFAULT }

	virtual void PutEmails(Signal::SlotRef slot, const MojObject::ObjectVec& array) { DEFAULT }
	virtual void MergeFlags(Signal::SlotRef slot, const MojObject::ObjectVec& objects) { DEFAULT }
	virtual void UpdateEmails(Signal::SlotRef slot, const MojObject::ObjectVec& objects) { DEFAULT }
	virtual void DeleteEmailIds(Signal::SlotRef slot, const MojObject::ObjectVec& ids) { DEFAULT }

	virtual void UpdateEmail(Signal::SlotRef slot, const MojObject& obj) { DEFAULT }
	virtual void UpdateFolder(Signal::SlotRef slot, const MojObject& obj) { DEFAULT }

	//----
	virtual void Reply(Signal::SlotRef slot, MojObject response, MojErr err = MojErrNone)
	{
		MojRefCountedPtr<CannedReply> reply(new CannedReply(response, err));
		reply->HandleRequest(slot);
	}

	virtual void SetResponse(const char* func)
	{
		MojRefCountedPtr<RequestHandler> doNothing; // null
		m_cannedResponses[func] = doNothing;
	}

	virtual void SetResponse(const char* func, MojObject response, MojErr err = MojErrNone)
	{
		MojRefCountedPtr<CannedReply> reply(new CannedReply(response, err));
		m_cannedResponses[func] = reply;
	}

	virtual void Default(const char* func, Signal::SlotRef slot)
	{
		std::map< std::string, MojRefCountedPtr<RequestHandler> >::iterator it;

		if((it = m_cannedResponses.find(func)) != m_cannedResponses.end()) {
			if(it->second.get())
				it->second->HandleRequest(slot);
			m_cannedResponses.erase(it);
			return;
		}

		std::string msg = "mock " + std::string(func) + " not implemented";
		throw MailException(msg.c_str(), __FILE__, __LINE__);
	}

protected:
	class RequestHandler : public MojSignalHandler
	{
	public:
		virtual void HandleRequest(Signal::SlotRef slot) = 0;
	};

	class CannedReply : public RequestHandler
	{
	public:
		CannedReply(const MojObject& response, MojErr err) : m_signal(this), m_response(response), m_err(err) {}
		virtual ~CannedReply() {}

		void HandleRequest(Signal::SlotRef slot)
		{
			m_signal.connect(slot);
			m_signal.fire(m_response, m_err);
		}
	protected:
		MojDbClient::Signal m_signal;
		MojObject m_response;
		MojErr m_err;
	};

	std::map< std::string, MojRefCountedPtr<RequestHandler> > m_cannedResponses;
};

#endif /*MOCKDATABASE_H_*/
