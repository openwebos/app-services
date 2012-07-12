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

#include "commands/SearchFolderCommand.h"
#include "commands/FetchNewHeadersCommand.h"
#include "client/ImapSession.h"
#include "client/SearchRequest.h"
#include "data/ImapEmail.h"
#include "data/ImapEmailAdapter.h"
#include "protocol/FetchResponseParser.h"
#include "protocol/UidSearchResponseParser.h"
#include "exceptions/ExceptionUtils.h"
#include <sstream>
#include "ImapPrivate.h"

SearchFolderCommand::SearchFolderCommand(ImapSession& session, const MojObject& folderId, const MojRefCountedPtr<SearchRequest>& searchRequest)
: ImapSessionCommand(session),
  m_folderId(folderId),
  m_searchRequest(searchRequest),
  m_handleContinuationSlot(this, &SearchFolderCommand::HandleContinuation),
  m_searchResponseSlot(this, &SearchFolderCommand::HandleSearchResponse),
  m_headersResponseSlot(this, &SearchFolderCommand::HandleHeadersResponse)
{
}

SearchFolderCommand::~SearchFolderCommand()
{
}

void SearchFolderCommand::RunImpl()
{
	CommandTraceFunction();

	bool sendLiteralNow = false;

	stringstream ss;

	ss << "UID SEARCH CHARSET UTF-8 NOT DELETED TEXT {" << m_searchRequest->GetSearchText().length();

	if (m_session.GetCapabilities().HasCapability("LITERAL+")) {
		ss << "+";
		sendLiteralNow = true;
	}

	ss << "}";

	m_searchResponseParser.reset(new UidSearchResponseParser(m_session, m_searchResponseSlot, m_matchingUIDs));
	m_session.SendRequest(ss.str(), m_searchResponseParser);

	if (sendLiteralNow) {
		HandleContinuation();
	} else {
		m_searchResponseParser->SetContinuationResponseSlot(m_handleContinuationSlot);
	}
}

MojErr SearchFolderCommand::HandleContinuation()
{
	CommandTraceFunction();

	try {
		OutputStreamPtr& os = m_session.GetOutputStream();
		os->Write(m_searchRequest->GetSearchText());
		os->Write("\r\n");
		os->Flush();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

MojErr SearchFolderCommand::HandleSearchResponse()
{
	CommandTraceFunction();

	try {
		m_searchResponseParser->CheckStatus();

		int limit = m_searchRequest->GetLimit();
		int found = m_matchingUIDs.size();

		sort(m_matchingUIDs.begin(), m_matchingUIDs.end());

		if (limit > 0 && found > limit) {
			MojLogInfo(m_log, "search results: %d emails found (limiting to %d)", found, limit);

			m_matchingUIDs.erase(m_matchingUIDs.begin(), m_matchingUIDs.end() - limit);
		} else {
			MojLogInfo(m_log, "search results: %d emails found", m_matchingUIDs.size());
		}

		RequestHeaders();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SearchFolderCommand::RequestHeaders()
{
	CommandTraceFunction();

	stringstream ss;

	ss << "UID FETCH ";
	AppendUIDs(ss, m_matchingUIDs.rbegin(), m_matchingUIDs.rend());
	ss << " (" << FetchNewHeadersCommand::FETCH_ITEMS << ")";

	m_headersResponseParser.reset(new FetchResponseParser(m_session, m_headersResponseSlot));
	m_session.SendRequest(ss.str(), m_headersResponseParser);
}

MojErr SearchFolderCommand::HandleHeadersResponse()
{
	CommandTraceFunction();

	try {
		MojErr err;
		MojObject response;

		MojObject results;

		BOOST_FOREACH(const FetchUpdate& update, m_headersResponseParser->GetUpdates()) {
			const boost::shared_ptr<ImapEmail>& email = update.email;

			if (email.get() && !email->IsDeleted()) {
				MojObject emailObj;

				// Set folderId
				email->SetFolderId(m_folderId);

				ImapEmailAdapter::SerializeToDatabaseObject(*email, emailObj);
				err = results.push(emailObj);
				ErrorToException(err);
			}
		}

		err = response.put("results", results);
		ErrorToException(err);

		if (m_searchRequest->GetServiceMessage().get()) {
			m_searchRequest->GetServiceMessage()->replySuccess(response);
			m_searchRequest->SetServiceMessage(NULL);
		}

		Done();

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void SearchFolderCommand::Done()
{
	Complete();
}

bool SearchFolderCommand::Cancel(CancelType cancelReason)
{
	if (m_searchRequest->GetServiceMessage().get()) {
		MailError::ErrorInfo errorInfo = GetCancelErrorInfo(cancelReason);

		m_searchRequest->GetServiceMessage()->replyError( (MojErr) errorInfo.errorCode, errorInfo.errorText.c_str());
		m_searchRequest->SetServiceMessage(NULL);
	}

	if (IsRunning()) {
		Complete();
	}

	return true;
}

void SearchFolderCommand::Failure(const std::exception& e)
{
	if (m_searchRequest->GetServiceMessage().get()) {
		MailError::ErrorInfo errorInfo = ExceptionUtils::GetErrorInfo(e);

		m_searchRequest->GetServiceMessage()->replyError( (MojErr) errorInfo.errorCode, errorInfo.errorText.c_str());
		m_searchRequest->SetServiceMessage(NULL);
	}

	ImapSessionCommand::Failure(e);
}
