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

#ifndef RECONCILEEMAILSCOMMAND_H_
#define RECONCILEEMAILSCOMMAND_H_

#include "commands/PopSessionCommand.h"
#include "commands/LoadUidCacheCommand.h"
#include "core/MojObject.h"
#include "data/PopAccount.h"
#include "data/PopEmail.h"
#include "data/UidCache.h"
#include "data/UidMap.h"
#include "db/MojDbClient.h"

class UidMap;
class SyncEmailsCommand;

class ReconcileEmailsCommand : public PopSessionCommand
{
public:
	typedef MojSignal1<bool>	DoneSignal;
	static const int LOAD_EMAIL_BATCH_SIZE;  // batch size to load emails from database

	enum MessageStatus {
		Status_Fetch_Header,
		Status_Downloaded,
		Status_Old_Email,
		Status_None
	};

	class ReconcileInfo {
	public:
		ReconcileInfo() : m_status(Status_None), m_timestamp(0) { }
		~ReconcileInfo() {}

		void SetMessageInfo(const UidMap::MessageInfo& info){ m_msgInfo = info; }
		void SetStatus(MessageStatus status)				{ m_status = status; }
		void SetTimestamp(const MojInt64& ts)				{ m_timestamp = ts; }

		const UidMap::MessageInfo&	GetMessageInfo() const	{ return m_msgInfo; }
		MessageStatus 				GetStatus()				{ return m_status; }
		const MojInt64& 			GetTimestamp() const 	{ return m_timestamp; }
	private:
		UidMap::MessageInfo			m_msgInfo;
		MessageStatus				m_status;
		MojInt64					m_timestamp;

	};
	typedef boost::shared_ptr<ReconcileInfo>	ReconcileInfoPtr;
	typedef std::queue<ReconcileInfoPtr>	 	ReconcileInfoQueue;

	struct LocalDeletedEmailInfo {
		MojObject	m_id;
		std::string m_uid;

		LocalDeletedEmailInfo() {}
		LocalDeletedEmailInfo(const MojObject& id, const std::string& uid) : m_id(id), m_uid(uid) {}

		bool operator==(const LocalDeletedEmailInfo& rhs) const { return m_id == rhs.m_id && m_uid == rhs.m_uid; }
		bool operator<(const LocalDeletedEmailInfo& rhs) const  { return m_uid < rhs.m_uid; }
	};
	typedef std::vector<LocalDeletedEmailInfo> LocalDeletedEmailsVec;

	ReconcileEmailsCommand(PopSession& session,
			const MojObject& folderId,
			const MojInt64& cutoffTime,
			MojInt64& latestEmailTimestamp,
			int& messageCount,
			ReconcileInfoQueue& queue,
			UidCache& uidCache,
			MojObject::ObjectVec& oldEmailIds,
			MojObject::ObjectVec& serverDeletedEmailIds,
			LocalDeletedEmailsVec& localDeletedEmailUids);
	~ReconcileEmailsCommand();

	void RunImpl();
	const ReconcileInfoQueue& GetReconcileInfoQueue() 	{ return m_reconcileQueue; }

	MojErr	GetLocalEmailsResponse(MojObject& response, MojErr err);
	MojErr	GetDeletedEmailsResponse(MojObject& response, MojErr err);
	MojErr	GetUidCacheResponse();
private:
	typedef std::map<std::string, ReconcileInfoPtr> ReconcileInfoUidMap;
	typedef std::map<int, ReconcileInfoPtr>		 	ReconcileInfoMsgNumMap;

	class UidMsgNumPair {
	public:
		UidMsgNumPair(std::string uid, int num) : m_uid(uid), m_msgNum(num) { }
		~UidMsgNumPair() {}

		const std::string& GetUid() const	{ return m_uid; }
		int GetMessageNumber() const		{ return m_msgNum; }
		bool operator< (const UidMsgNumPair& rhs)	const	{ return m_msgNum < rhs.m_msgNum; }
	private:
		std::string m_uid;
		int			m_msgNum;
	};

	void InitReconcileMap(boost::shared_ptr<UidMap> uidMap);
	void GetLocalEmails();
	void GetLocalDeletedEmails();
	void GetUidCache();
	void ReconcileEmails();

	boost::shared_ptr<PopAccount>						m_account;
	MojObject 											m_folderId;
	MojInt64											m_cutOffTime;
	MojInt64&											m_latestEmailTimestamp;
	int&												m_messageCount;
	const boost::shared_ptr<UidMap>&					m_uidMap;
	UidCache&											m_uidCache;
	ReconcileInfoUidMap									m_reconcileUidMap;
	ReconcileInfoMsgNumMap								m_reconcileMsgNumMap;
	ReconcileInfoQueue&									m_reconcileQueue;
	MojObject::ObjectVec&								m_oldEmailIds;
	MojObject::ObjectVec&								m_serverDeletedEmailIds;
	LocalDeletedEmailsVec&								m_localDeletedEmailUids;
	MojDbQuery::Page									m_localEmailsPage;
	bool												m_failed;

	MojRefCountedPtr<LoadUidCacheCommand>				m_loadCacheCommand;
	MojRefCountedPtr<PopCommandResult>					m_commandResult;

	MojDbClient::Signal::Slot<ReconcileEmailsCommand> 	m_getLocalEmailsResponseSlot;
	MojSignal<>::Slot<ReconcileEmailsCommand> 			m_getUidCacheResponseSlot;
	MojDbClient::Signal::Slot<ReconcileEmailsCommand> 	m_getDeletedEmailsResponseSlot;
};

#endif /* RECONCILEEMAILSCOMMAND_H_ */
