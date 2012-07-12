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

#include "commands/SyncEmailsCommand.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian_types.hpp>
#include <sstream>
#include "client/ImapSession.h"
#include "commands/AutoDownloadCommand.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "data/ImapEmailAdapter.h"
#include "data/ImapEmail.h"
#include "data/EmailSchema.h"
#include "ImapPrivate.h"
#include "sync/UIDMap.h"
#include "protocol/UidSearchResponseParser.h"
#include "protocol/FetchResponseParser.h"
#include "client/FolderSession.h"
#include <algorithm>
#include "sync/SyncEngine.h"
#include <utility>
#include "commands/ImapCommandResult.h"
#include "commands/SyncLocalChangesCommand.h"
#include "client/SyncSession.h"
#include "ImapConfig.h"
#include "commands/FetchNewHeadersCommand.h"

using namespace boost::gregorian;

SyncEmailsCommand::SyncEmailsCommand(ImapSession& session, const MojObject& folderId, SyncParams syncParams)
: ImapSyncSessionCommand(session, folderId),
  m_syncParams(syncParams),
  m_syncLocalChangesSlot(this, &SyncEmailsCommand::SyncLocalChangesDone),
  m_searchResponseSlot(this, &SyncEmailsCommand::SearchResponse),
  m_fetchResponseSlot(this, &SyncEmailsCommand::FetchResponse),
  m_getLocalEmailsResponseSlot(this, &SyncEmailsCommand::GetLocalEmailsResponse),
  m_putEmailsResponseSlot(this, &SyncEmailsCommand::PutEmailsResponse),
  m_deleteLocalEmailsResponseSlot(this, &SyncEmailsCommand::DeleteLocalEmailsResponse),
  m_mergeFlagsResponseSlot(this, &SyncEmailsCommand::MergeFlagsResponse),
  m_autoDownloadSlot(this, &SyncEmailsCommand::AutoDownloadDone)
{
	assert(!folderId.undefined() && !folderId.null());
}

SyncEmailsCommand::~SyncEmailsCommand()
{
}

void SyncEmailsCommand::RunImpl()
{
	CommandTraceFunction();
	// Get sync window from preferences.
	assert( m_session.GetAccount().get() != NULL );
	m_daysBack = m_session.GetAccount()->GetSyncWindowDays();
	SyncLocalChanges();
}

void SyncEmailsCommand::SyncLocalChanges()
{
	CommandTraceFunction();

	m_syncLocalChangesCommand.reset(new SyncLocalChangesCommand(m_session, m_folderId));
	m_syncLocalChangesCommand->Run(m_syncLocalChangesSlot);
}

MojErr SyncEmailsCommand::SyncLocalChangesDone()
{
	CommandTraceFunction();

	try {
		m_syncLocalChangesCommand->GetResult()->CheckException();

		SyncServerChanges();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncEmailsCommand::SyncServerChanges()
{
	CommandTraceFunction();

	try {
		m_pendingSearches.push(ALL); // this one is essential
		m_pendingSearches.push(DELETED);
		m_pendingSearches.push(UNSEEN);
		m_pendingSearches.push(ANSWERED);
		m_pendingSearches.push(FLAGGED);

		Search();
	} CATCH_AS_FAILURE
}

// TODO move this to another class
string SinceDateString(int daysBack)
{
	date d = day_clock::universal_day() - days(daysBack);

	stringstream ss;
	date_facet *df = new date_facet("%d-%b-%Y"); // will be freed by locale
	locale loc = locale(locale("C"), df);
	ss.imbue(loc);
	ss << d;
	
	return ss.str();
}

void SyncEmailsCommand::Search()
{
	CommandTraceFunction();

	string currentSearch;

	SearchType searchType = m_pendingSearches.front();
	m_pendingSearches.pop();

	std::vector<UID>* targetList;

	switch(searchType) {
	case ALL:
		currentSearch = "ALL";
		targetList = &m_allUIDs;
		break;
	case DELETED:
		currentSearch = "DELETED";
		targetList = &m_deletedUIDs;
		break;
	case UNSEEN:
		currentSearch = "UNSEEN";
		targetList = &m_unseenUIDs;
		break;
	case ANSWERED:
		currentSearch = "ANSWERED";
		targetList = &m_answeredUIDs;
		break;
	case FLAGGED:
		currentSearch = "FLAGGED";
		targetList = &m_flaggedUIDs;
		break;
	default:
		assert(0);
		throw MailException("bad enum value", __FILE__, __LINE__);
	}

	stringstream ss;
	ss << "UID SEARCH " << currentSearch;

	if(m_daysBack > 0) {
		// If daysBack is 0, get all emails, otherwise add the SINCE parameter
		ss << " SINCE " << SinceDateString(m_daysBack);
	}

	m_responseParser.reset(new UidSearchResponseParser(m_session, m_searchResponseSlot, *targetList));
	m_session.SendRequest(ss.str(), m_responseParser);
}

MojErr SyncEmailsCommand::SearchResponse()
{
	CommandTraceFunction();

	try {
		// Check error
		m_responseParser->CheckStatus();

		// Run next search, or finish searches
		if(!m_pendingSearches.empty()) {
			Search();
		} else {
			SearchComplete();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncEmailsCommand::SearchComplete()
{
	CommandTraceFunction();

	boost::shared_ptr<FolderSession> folderSession = m_session.GetFolderSession();
	assert( folderSession.get() );

	// Create UID map
	sort(m_allUIDs.begin(), m_allUIDs.end());

	unsigned int maxSize = ImapConfig::GetConfig().GetMaxEmails();

	m_uidMap = make_shared<UIDMap>(m_allUIDs, folderSession->GetMessageCount(), maxSize);
	folderSession->SetUIDMap(m_uidMap);

	// Sort lists
	sort(m_deletedUIDs.begin(), m_deletedUIDs.end());
	sort(m_unseenUIDs.begin(), m_unseenUIDs.end());
	sort(m_answeredUIDs.begin(), m_answeredUIDs.end());
	sort(m_flaggedUIDs.begin(), m_flaggedUIDs.end());

	// Initialize sync engine with sorted lists
	m_syncEngine.reset(new SyncEngine(m_uidMap->GetUIDs(), m_deletedUIDs, m_unseenUIDs, m_answeredUIDs, m_flaggedUIDs));

	GetLocalEmails();
}

void SyncEmailsCommand::GetLocalEmails()
{
	CommandTraceFunction();
	
	m_session.GetDatabaseInterface().GetEmailSyncList(m_getLocalEmailsResponseSlot, m_folderId, m_localEmailsPage);
}

MojErr SyncEmailsCommand::GetLocalEmailsResponse(MojObject& response, MojErr err)
{
	vector<SyncEngine::EmailStub> localEmails;

	try {
		// Check database response
		ErrorToException(err);
		
		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		MojObject::ArrayIterator it;
		err = results.arrayBegin(it);
		ErrorToException(err);

		// Read email properties: _id, UID, flags
		// If you need additional properties, modify the database query
		for (; it != results.arrayEnd(); ++it) {
			MojObject& emailObj = *it;

			MojObject id;
			err = emailObj.getRequired(DatabaseAdapter::ID, id);
			ErrorToException(err);
			
			UID uid;
			err = emailObj.getRequired(ImapEmailAdapter::UID, uid);
			ErrorToException(err);
			
			EmailFlags lastSyncFlags;

			if(emailObj.contains(ImapEmailAdapter::LAST_SYNC_FLAGS)) {
				MojObject lastSyncFlagsObj;
				err = emailObj.getRequired(ImapEmailAdapter::LAST_SYNC_FLAGS, lastSyncFlagsObj);
				ErrorToException(err);

				ImapEmailAdapter::ParseEmailFlags(lastSyncFlagsObj, lastSyncFlags);
			}

			// Add to list -- database results must be sorted in ascending UID order
			SyncEngine::EmailStub stub(uid, id);
			stub.lastSyncFlags = lastSyncFlags;

			localEmails.push_back(stub);
		}

		bool hasMore = DatabaseAdapter::GetNextPage(response, m_localEmailsPage);

		assert( m_syncEngine.get() != NULL );
		m_syncEngine->Diff(localEmails, hasMore);

		if(hasMore) {
			// Get next batch of emails
			MojLogDebug(m_log, "getting another batch of local emails");
			GetLocalEmails();
		} else {
			// After all changes are accounted for, handle fetching new messages and updating/deleting existing messages
			SyncUpdatesFromServer();
		}

	} CATCH_AS_FAILURE
	
	return MojErrNone;
}

void SyncEmailsCommand::SyncUpdatesFromServer()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "%d new emails, %d deleted emails, %d changed emails on server",
			m_syncEngine->GetNewUIDs().size(), m_syncEngine->GetDeletedIds().size(), m_syncEngine->GetModifiedFlags().size());

	// Order is important here!
	if(!m_syncEngine->GetNewUIDs().empty())
		FetchNewMessages();
	else if(!m_syncEngine->GetDeletedIds().empty())
		DeleteLocalEmails();
	else if(!m_syncEngine->GetModifiedFlags().empty())
		MergeFlags();
	else
		AutoDownload();
}

void SyncEmailsCommand::FetchNewMessages()
{
	CommandTraceFunction();
	
	// Copy to queue
	m_pendingHeaders.insert(m_pendingHeaders.begin(), m_syncEngine->GetNewUIDs().begin(), m_syncEngine->GetNewUIDs().end());

	FetchOneBatch();
}

void SyncEmailsCommand::FetchOneBatch()
{
	CommandTraceFunction();

	stringstream ss;
	ss << "UID FETCH ";

	int batchSize = m_session.IsSafeMode() ? 1 : ImapConfig::GetConfig().GetHeaderBatchSize();

	int count = 0;
	while(!m_pendingHeaders.empty() && count < batchSize) {
		UID uid = m_pendingHeaders.back();
		m_pendingHeaders.pop_back();

		if(count > 0) { // first item in set
			ss << ",";
		}

		ss << uid;
		count++;
	}

	ss << " (" << FetchNewHeadersCommand::FETCH_ITEMS << ")";

	string command = ss.str();

	m_fetchResponseParser.reset(new FetchResponseParser(m_session, m_fetchResponseSlot));
	m_session.SendRequest(command, m_fetchResponseParser);
}

MojErr SyncEmailsCommand::FetchResponse()
{
	CommandTraceFunction();
	
	MojErr err;
	MojObject::ObjectVec array;

	try {
		try {
			m_fetchResponseParser->CheckStatus();
		} catch(const Rfc3501ParseException& e) {
			m_session.SetSafeMode(true);
			throw; // rethrow exception
		}

		BOOST_FOREACH(const FetchUpdate& update, m_fetchResponseParser->GetUpdates()) {
			const boost::shared_ptr<ImapEmail>& email = update.email;

			assert( email.get() != NULL );
			assert( IsValidId(m_folderId) );

			// Make sure this is a valid email
			if (email->GetUID() == 0 || email->IsDeleted()) {
				continue;
			}

			// Set folderId
			email->SetFolderId(m_folderId);

			// Set autodownload
			email->SetAutoDownload(true);

			MojObject emailObj;
			ImapEmailAdapter::SerializeToDatabaseObject(*email.get(), emailObj);

			err = array.push(emailObj);
			ErrorToException(err);
		}
		m_session.GetDatabaseInterface().PutEmails(m_putEmailsResponseSlot, array);

	} CATCH_AS_FAILURE

	return MojErrNone;
}

MojErr SyncEmailsCommand::PutEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		if(err != MojErrNone) {
			// Possible UTF-8 error
			m_session.SetSafeMode(true);
		}

		ErrorToException(err);

		m_syncSession->AddPutResponseRevs(response);

		if(!m_pendingHeaders.empty()) {
			FetchOneBatch();
		} else {
			DeleteLocalEmails();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncEmailsCommand::DeleteLocalEmails()
{
	CommandTraceFunction();

	MojObject::ObjectVec deleted;

	BOOST_FOREACH(const MojObject& id, m_syncEngine->GetDeletedIds()) {
		deleted.push(id);
	}

	if(!deleted.empty())
		m_session.GetDatabaseInterface().DeleteEmailIds(m_deleteLocalEmailsResponseSlot, deleted);
	else
		MergeFlags();
}

MojErr SyncEmailsCommand::DeleteLocalEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		m_syncSession->AddPutResponseRevs(response);

		MergeFlags();
	} CATCH_AS_FAILURE
	
	return MojErrNone;
}

void SyncEmailsCommand::MergeFlags()
{
	CommandTraceFunction();

	typedef std::pair<SyncEngine::EmailStub, EmailFlags> UpdatedFlags;

	MojObject::ObjectVec modified;

	BOOST_FOREACH(const UpdatedFlags& p, m_syncEngine->GetModifiedFlags()) {
		const SyncEngine::EmailStub& stub = p.first;
		const EmailFlags& serverFlags = p.second;

		MojErr err;

		MojObject obj;
		err = obj.put("_id", stub.id); // FIXME use constant
		ErrorToException(err);

		MojObject flagsObj;
		if(stub.lastSyncFlags.read != serverFlags.read) {
			err = flagsObj.put(EmailSchema::Flags::READ, (bool) serverFlags.read);
			ErrorToException(err);
		}
		if(stub.lastSyncFlags.replied != serverFlags.replied) {
			err = flagsObj.put(EmailSchema::Flags::REPLIED, (bool) serverFlags.replied);
			ErrorToException(err);
		}
		if(stub.lastSyncFlags.flagged != serverFlags.flagged) {
			err = flagsObj.put(EmailSchema::Flags::FLAGGED, (bool) serverFlags.flagged);
			ErrorToException(err);
		}

		err = obj.put("flags", flagsObj);
		ErrorToException(err);
		err = obj.put("lastSyncFlags", flagsObj);
		ErrorToException(err);

		// FIXME include rev

		modified.push(obj);
	}

	m_session.GetDatabaseInterface().MergeFlags(m_mergeFlagsResponseSlot, modified);
}

MojErr SyncEmailsCommand::MergeFlagsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		m_syncSession->AddPutResponseRevs(response);

		AutoDownload();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SyncEmailsCommand::AutoDownload()
{
	CommandTraceFunction();

	// Check for bodies to download
	m_autoDownloadCommand.reset(new AutoDownloadCommand(m_session, m_folderId));
	m_autoDownloadCommand->Run(m_autoDownloadSlot);
}

MojErr SyncEmailsCommand::AutoDownloadDone()
{
	CommandTraceFunction();

	try {
		m_autoDownloadCommand->GetResult()->CheckException();

		// If we got this far succesfully, disable safe mode
		m_session.SetSafeMode(false);

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

bool SyncEmailsCommand::Equals(const ImapCommand& other) const
{
	if(other.GetType() != CommandType_Sync) {
		return false;
	}

	const SyncEmailsCommand& otherSync = static_cast<const SyncEmailsCommand&>(other);
	if(m_folderId == otherSync.m_folderId) {
		return true;
	}

	return false;
}

std::string SyncEmailsCommand::Describe() const
{
	return GetClassName() + " [folderId=" + AsJsonString(m_folderId) + "]";
}

void SyncEmailsCommand::Status(MojObject& status) const
{
	MojErr err;

	ImapSyncSessionCommand::Status(status);

	MojObject syncParamsStatus;
	m_syncParams.Status(syncParamsStatus);
	err = status.put("syncParams", syncParamsStatus);
	ErrorToException(err);

	if(m_syncLocalChangesCommand.get()) {
		MojObject commandStatus;
		m_syncLocalChangesCommand->Status(commandStatus);
		err = status.put("syncLocalChangesCommand", commandStatus);
		ErrorToException(err);
	}
}
