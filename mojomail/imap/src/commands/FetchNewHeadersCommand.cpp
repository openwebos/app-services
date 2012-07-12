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

#include "commands/FetchNewHeadersCommand.h"
#include "protocol/FetchResponseParser.h"
#include "client/FolderSession.h"
#include "client/ImapSession.h"
#include "data/DatabaseInterface.h"
#include "data/ImapEmail.h"
#include "data/ImapEmailAdapter.h"
#include "sync/UIDMap.h"
#include <sstream>
#include "ImapPrivate.h"
#include "ImapConfig.h"

const std::string FetchNewHeadersCommand::FETCH_ITEMS = "UID FLAGS INTERNALDATE ENVELOPE BODYSTRUCTURE BODY.PEEK[HEADER.FIELDS (X-PRIORITY IMPORTANCE)]";

// FIXME should be a sync session command
FetchNewHeadersCommand::FetchNewHeadersCommand(ImapSession& session, const MojObject& folderId)
: ImapSessionCommand(session),
  m_folderId(folderId),
  m_fetchSlot(this, &FetchNewHeadersCommand::FetchResponse),
  m_putEmailsSlot(this, &FetchNewHeadersCommand::PutEmailsResponse)
{
}

FetchNewHeadersCommand::~FetchNewHeadersCommand()
{
}

void FetchNewHeadersCommand::RunImpl()
{
	const boost::shared_ptr<FolderSession>& folderSession = m_session.GetFolderSession();

	if(folderSession.get() && folderSession->HasUIDMap()) {
		folderSession->GetUIDMap()->GetMissingMsgNums(m_msgNums);

		int totalEmails = folderSession->GetUIDMap()->GetUIDs().size();

		// If we're way over the max number of emails, force a full sync to avoid unbounded growth (DFISH-9737)
		if (totalEmails > ImapConfig::GetConfig().GetMaxEmails() * 1.1) {
			MojLogInfo(m_log, "exceeded email limit; requesting full sync");
			m_msgNums.clear();

			SyncParams params;
			params.SetForce(true);
			params.SetReason("exceeded email limit");

			m_session.SyncFolder(m_folderId, params);
		}
	} else {
		// If we don't have a UID map, we might have gotten disconnected from the server.
		// Schedule a sync.
		SyncParams params;
		params.SetForce(false);
		params.SetReason("didn't have a UID map while fetching new headers");

		m_session.SyncFolder(m_folderId, params);
	}

	if(!m_msgNums.empty()) {
		FetchNewHeaders();
	} else {
		Complete();
	}
}

void FetchNewHeadersCommand::FetchNewHeaders()
{
	CommandTraceFunction();

	m_fetchResponseParser.reset(new FetchResponseParser(m_session, m_fetchSlot));

	stringstream ss;
	ss << "FETCH ";

	// Technically not UIDs, but right now they're both unsigned ints
	AppendUIDs(ss, m_msgNums.begin(), m_msgNums.end());

	// TODO merge this code with the fetch in SyncEmailsCommand (but must request "UID")
	// Note, this uses a normal FETCH, not UID FETCH
	ss << " (" << FETCH_ITEMS << ")";
	//ss << " (UID FLAGS INTERNALDATE ENVELOPE BODYSTRUCTURE)";

	m_session.SendRequest(ss.str(), m_fetchResponseParser);
}

MojErr FetchNewHeadersCommand::FetchResponse()
{
	CommandTraceFunction();

	boost::shared_ptr<UIDMap> uidMap;

	const boost::shared_ptr<FolderSession>& folderSession = m_session.GetFolderSession();
	if(folderSession.get() && folderSession->HasUIDMap()) {
		uidMap = folderSession->GetUIDMap();
	}

	try {
		m_fetchResponseParser->CheckStatus();

		MojObject::ObjectVec array;

		BOOST_FOREACH(const FetchUpdate& update, m_fetchResponseParser->GetUpdates()) {
			const boost::shared_ptr<ImapEmail>& email = update.email;

			assert( email.get() != NULL );
			assert( IsValidId(m_folderId) );

			if(update.msgNum > 0 && email->GetUID() > 0 && uidMap.get()) {
				// Update UID map
				uidMap->SetMsgUID(update.msgNum, email->GetUID());
			} else {
				MojLogWarning(m_log, "unable to update UID map after fetch response: uid=%d, msgNum=%d, uidMap=%p",
						email->GetUID(), update.msgNum, uidMap.get());
			}

			// Add email, only if it has a valid UID and is not a deleted email
			if(email->GetUID() > 0 && !email->IsDeleted()) {
				// Set folderId
				email->SetFolderId(m_folderId);

				// Set autodownload
				email->SetAutoDownload(true);

				MojObject emailObj;
				ImapEmailAdapter::SerializeToDatabaseObject(*email.get(), emailObj);
				array.push(emailObj);
			}
		}

		m_session.GetDatabaseInterface().PutEmails(m_putEmailsSlot, array);
	} CATCH_AS_FAILURE

	return MojErrNone;
}

MojErr FetchNewHeadersCommand::PutEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		// Queue auto-download command
		m_session.AutoDownloadParts(m_folderId);

		Complete();

	} CATCH_AS_FAILURE

	return MojErrNone;
}
