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

#ifndef SyncEmailsCommand_H_
#define SyncEmailsCommand_H_

#include "commands/ImapSyncSessionCommand.h"
#include "db/MojDbClient.h"
#include "ImapCoreDefs.h"
#include "protocol/ImapResponseParser.h"
#include <vector>
#include <queue>
#include <algorithm>
#include "sync/SyncEngine.h"
#include <utility>
#include <boost/scoped_ptr.hpp>
#include "client/SyncParams.h"

class ImapEmail;
class UIDMap;
class UidSearchResponseParser;
class FetchResponseParser;
class SyncLocalChangesCommand;
class AutoDownloadCommand;

class SyncEmailsCommand : public ImapSyncSessionCommand
{
public:
	SyncEmailsCommand(ImapSession& session, const MojObject& folderId, SyncParams syncParams = SyncParams());
	virtual ~SyncEmailsCommand();
	
	void RunImpl();

	void SyncLocalChanges();
	MojErr SyncLocalChangesDone();

	void SyncServerChanges();

	void Search();
	MojErr SearchResponse();
	void SearchComplete();

	void GetLocalEmails();
	MojErr GetLocalEmailsResponse(MojObject& response, MojErr err);

	// Fetch new emails, delete removed emails, update flags
	void SyncUpdatesFromServer();

	void FetchNewMessages();
	void FetchOneBatch();
	MojErr FetchResponse();
	
	MojErr PutEmailsResponse(MojObject& response, MojErr err);

	void DeleteLocalEmails();
	MojErr DeleteLocalEmailsResponse(MojObject& response, MojErr err);

	void MergeFlags();
	MojErr MergeFlagsResponse(MojObject& response, MojErr err);

	void AutoDownload();
	MojErr AutoDownloadDone();

	void Status(MojObject& status) const;

	std::string Describe() const;

	CommandType GetType() const { return CommandType_Sync; }
	bool Equals(const ImapCommand& other) const;

protected:
	int			m_daysBack;

	boost::shared_ptr<UIDMap>	m_uidMap;
	
	enum SearchType
	{
		ALL, DELETED, UNSEEN, ANSWERED, FLAGGED
	};

	MojRefCountedPtr<SyncLocalChangesCommand> m_syncLocalChangesCommand;
	MojRefCountedPtr<AutoDownloadCommand> m_autoDownloadCommand;

	std::queue<SearchType> m_pendingSearches;

	MojDbQuery::Page	m_localEmailsPage;

	boost::scoped_ptr<SyncEngine>	m_syncEngine;

	MojRefCountedPtr<UidSearchResponseParser>	m_responseParser;
	MojRefCountedPtr<FetchResponseParser>		m_fetchResponseParser;
	
	std::vector<UID>	m_allUIDs, m_deletedUIDs, m_unseenUIDs, m_answeredUIDs, m_flaggedUIDs;

	std::deque<UID>		m_pendingHeaders;

	SyncParams			m_syncParams;

	MojSignal<>::Slot<SyncEmailsCommand>		m_syncLocalChangesSlot;
	ImapResponseParser::DoneSignal::Slot<SyncEmailsCommand>	m_searchResponseSlot;
	ImapResponseParser::DoneSignal::Slot<SyncEmailsCommand>	m_fetchResponseSlot;
	MojDbClient::Signal::Slot<SyncEmailsCommand> m_getLocalEmailsResponseSlot;
	MojDbClient::Signal::Slot<SyncEmailsCommand> m_putEmailsResponseSlot;
	MojDbClient::Signal::Slot<SyncEmailsCommand> m_deleteLocalEmailsResponseSlot;
	MojDbClient::Signal::Slot<SyncEmailsCommand> m_mergeFlagsResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand>		m_autoDownloadSlot;
};

#endif /*SyncEmailsCommand_H_*/
