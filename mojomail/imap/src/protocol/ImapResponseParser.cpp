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

#include "protocol/ImapResponseParser.h"
#include "exceptions/MailException.h"
#include <boost/algorithm/string/predicate.hpp>
#include "client/ImapSession.h"
#include "exceptions/ExceptionUtils.h"

using namespace std;

ImapResponseParser::ImapResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot)
: m_session(session),
  m_log(session.GetLogger()),
  m_sessionWaiting(false),
  m_status(STATUS_UNKNOWN),
  m_doneSignal(this),
  m_continuationSignal(this)
{
	m_doneSignal.connect(doneSlot);
}

ImapResponseParser::ImapResponseParser(ImapSession& session)
: m_session(session),
  m_log(session.GetLogger()),
  m_sessionWaiting(false),
  m_status(STATUS_UNKNOWN),
  m_doneSignal(this),
  m_continuationSignal(this)
{
}

ImapResponseParser::~ImapResponseParser()
{
}

void ImapResponseParser::SplitOnce(const string& line, string& firstWord, string& rest)
{
	size_t wordLength = line.find(' ');

	if(wordLength != string::npos) {
		firstWord = line.substr(0, wordLength);

		// Skip over spaces
		size_t restStart = line.find_first_not_of(' ', wordLength);
		if(restStart != string::npos)
			rest = line.substr(restStart);
		else
			rest = "";
	} else {
		// The line is the word
		firstWord = line;
		rest = "";
	}
}

void ImapResponseParser::SplitResponse(const string& line, string& tag, ImapStatusCode& responseType, string& remaining)
{
	string rest;
	SplitOnce(line, tag, rest);
	
	if(tag.empty()) {
		throw MailException("parse exception in IMAP server response line: no tag", __FILE__, __LINE__);
	}
	
	string status;
	SplitOnce(rest, status, remaining);

	if(boost::iequals(status, "OK")) {
		responseType = OK;
	} else if(boost::iequals(status, "NO")) {
		responseType = NO;
	} else if(boost::iequals(status, "BAD")) {
		responseType = BAD;
	} else {
		throw MailException("unexpected status in IMAP server response line", __FILE__, __LINE__);
	}
}

void ImapResponseParser::SetResponse(ImapStatusCode status, const string& line)
{
	m_status = status;
	m_responseLine = line;
}

bool ImapResponseParser::HandleUntaggedResponse(const string& line)
{
	return false;
}

void ImapResponseParser::HandleResponse(ImapStatusCode status, const string& line)
{
}

void ImapResponseParser::SetContinuationResponseSlot(ContinuationSignal::SlotRef continuationSlot)
{
	m_continuationSignal.connect(continuationSlot);
}

bool ImapResponseParser::HandleContinuationResponse()
{
	m_continuationSignal.fire();
	return true;
}

void ImapResponseParser::BaseHandleResponse(ImapStatusCode status, const string& line)
{
	SetResponse(status, line);
	HandleResponse(status, line);
	Done();
}

bool ImapResponseParser::HandleAdditionalData()
{
	throw MailException("unhandled additional data", __FILE__, __LINE__);
}

void ImapResponseParser::CheckStatus()
{
	if(m_status != OK) {
		if(m_status == STATUS_UNKNOWN) {
			throw MailException("status not yet available", __FILE__, __LINE__);
		}

		if(m_status == STATUS_EXCEPTION) {
			boost::rethrow_exception(m_exception);
		}

		// FIXME use a more specific exception
		string msg = "server responded with error: " + m_responseLine;
		throw MailException(msg.c_str(), __FILE__, __LINE__);
	}
}

void ImapResponseParser::HandleError(const exception& e)
{
	m_status = STATUS_EXCEPTION;
	m_exception = ExceptionUtils::CopyException(e);

	Done();
}

void ImapResponseParser::Done()
{
	// Make sure we release the session
	if(m_sessionWaiting) {
		m_session.ParserFinished(this);
		m_sessionWaiting = false;
	}

	m_doneSignal.fire();
}
