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

#include "client/ImapRequestManager.h"
#include "client/ImapSession.h"
#include "protocol/UntaggedUpdateParser.h"
#include <sstream>
#include "ImapPrivate.h"

using namespace std;

MojLogger ImapRequestManager::s_protocolLog("com.palm.mail.protocol");

ImapRequestManager::ImapRequestManager(ImapSession& session)
: m_session(session),
  m_log(session.GetLogger()),
  m_currentRequestId(0),
  m_handlingResponses(false),
  m_waitingForLine(false),
  m_untaggedUpdateParser(new UntaggedUpdateParser(m_session)),
  m_handleResponsesSlot(this, &ImapRequestManager::HandleResponses),
  m_handleMoreDataSlot(this, &ImapRequestManager::HandleMoreData)
{
}

ImapRequestManager::~ImapRequestManager()
{
}

std::string ImapRequestManager::SendRequest(const std::string& request, bool canLogRequest)
{
	int id = ++m_currentRequestId;
	
	stringstream ss;
	
	// Create tag prefixed by "~A" from the current request id counter
	ss << "~A" << id;
	std::string tag = ss.str();
	
	// Combine tag and request, e.g. ~A15 CAPABILITY
	ss << " " << request << "\r\n";
	std::string line = ss.str();
	
	if(canLogRequest)
		MojLogDebug(s_protocolLog, "[session %p] sending: %s %s", &m_session, tag.c_str(), request.c_str());
	
	OutputStreamPtr& out = m_session.GetOutputStream();

	// note: this could throw an exception
	out->Write(line);
	out->Flush();
	
	return tag;
}

void ImapRequestManager::SendRequest(const string& request, const MojRefCountedPtr<ImapResponseParser>& responseParser, int timeoutSeconds, bool canLogRequest)
{
	assert( !request.empty() );
	assert( responseParser.get() );

	std::string tag = SendRequest(request, canLogRequest);
	m_pendingRequests.push_back( PendingRequest(tag, responseParser, timeoutSeconds) );

	// Don't call WaitForResponses if someone is already using the LineReader
	if(!Busy()) {
		WaitForResponses();
	}
}

void ImapRequestManager::UpdateRequestTimeout(const MojRefCountedPtr<ImapResponseParser>& responseParser, int timeoutSeconds)
{
	std::vector<PendingRequest>::iterator it;
	for(it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
		if(it->parser == responseParser) {
			it->timeout = timeoutSeconds;
		}
	}

	if(m_waitingForLine) {
		int newTimeout = GetNextTimeout();

		MojLogDebug(m_log, "updating request timeout to %d seconds", newTimeout);
		m_session.GetLineReader()->SetTimeout(newTimeout);
	} else {
		// If we're not waiting for a line, then either it'll start waiting soon, or
		// we shouldn't be messing with the line reader right now.
	}
}

int ImapRequestManager::GetNextTimeout()
{
	int timeout = 0;

	// Get the max timeout out of all the requests
	BOOST_FOREACH(const PendingRequest& request, m_pendingRequests) {
		if(request.timeout == 0) {
			timeout = 0;
			break;
		} else if(request.timeout > timeout) {
			timeout = request.timeout;
		}
	}

	return timeout;
}

void ImapRequestManager::WaitForResponses()
{
	MojLogTrace(m_log);
	m_waitingForLine = true;
	try {
		int timeout = GetNextTimeout();

		if(timeout > 0)
			MojLogDebug(m_log, "waiting for responses; timeout in %d seconds", timeout);
		else
			MojLogDebug(m_log, "waiting for responses; no timeout");

		MojRefCountedPtr<LineReader> lineReader = m_session.GetLineReader();
		lineReader->WaitForLine(m_handleResponsesSlot, timeout);
	} catch(const exception& e) {
		MojLogError(m_log, "exception waiting for responses: %s", e.what());
		ReportException(e);
	} catch(...) {
		MojLogError(m_log, "unknown exception waiting for responses");
		ReportException(UNKNOWN_EXCEPTION);
	}
}

/**
 * Handle response lines from server.
 * Typically the request/response goes like this: (C is client, S is server)
 * 
 * C: ~A1 UID FETCH 1:* FLAGS
 * S: * 1 FETCH (UID 2 FLAGS (\Seen))
 * S: * 2 FETCH (UID 7 FLAGS (\Seen))
 * S: * 3 FETCH (UID 8 FLAGS (\Seen))
 * S: ~A1 OK FETCH completed
 * 
 * The lines starting with '*' are untagged responses.
 * The line starting with ~A1 is a tagged response.
 * 
 * In general, we only execute one command at a time, so that the current
 * command can handle the untagged responses (otherwise, for commands like
 * SEARCH, we don't know which query triggered the untagged responses).
 * 
 * Note that we can also get extra unsolicited untagged responses from the server,
 * which don't correspond to the current command. Usually, it's something like
 * "* 15 EXISTS" or "* EXPUNGE 3" to indicate that the number of messages in the
 * currently selected folder has changed.
 * 
 * These events must be handled in order to accurately maintain the UID to message
 * number mapping.
 */
MojErr ImapRequestManager::HandleResponses()
{
	MojLogTrace(m_log);

	m_waitingForLine = false;
	m_handlingResponses = true;
	try {
		MojRefCountedPtr<LineReader> lineReader = m_session.GetLineReader();

		lineReader->CheckError();

		while(lineReader->MoreLinesInBuffer()) {
			string line = lineReader->ReadLine();
			MojLogDebug(s_protocolLog, "[session %p] response: %s", &m_session, line.c_str());

			HandleResponseLine(line);

			// If a parser is active, stop reading from the LineReader
			if(m_currentParser.get()) {
				break;
			}
		}

		// After we finish reading all lines from the buffer, return
		if(!m_pendingRequests.empty() && !m_currentParser.get()) {
			WaitForResponses();
		}

	} catch (const exception& e) {
		MojLogError(m_log, "%s: exception handling response: %s", __PRETTY_FUNCTION__, e.what());
		m_session.FatalError("exception in ImapRequestManager::HandleResponses");
		ReportException(e);
	} catch (...) {
		MojLogError(m_log, "unknown exception");
		ReportException(UNKNOWN_EXCEPTION);
	}

	m_handlingResponses = false;
	return MojErrNone;
}

/**
 * Handle one line of response.
 * 
 * @return whether more lines are expected
 */
void ImapRequestManager::HandleResponseLine(const string& line)
{
	if(line.length() > 0) {
		if(line.at(0) == '*' && line.length() > 2) {
			// untagged response
			bool handled = false;
			
			BOOST_FOREACH(const PendingRequest& request, m_pendingRequests) {
				handled = request.parser->HandleUntaggedResponse(line.substr(2));

				// Stop as soon as one parser claims to handle the response, or
				// if a untagged response handler needs to parse more data.
				if(handled || m_currentParser.get()) break;
			}

			if(!handled && m_currentParser.get() == NULL) {
				// TODO should call a generic untagged response handler
				HandleGenericUntaggedResponse(line.substr(2));
			}
		} else if(line.at(0) == '+') {
			// TODO: handle continuation by passing it to the first response handler.
			//       The response handler would probably start sending data to the server.
			// For now, just ignore it
			bool handled = false;
			if(!m_pendingRequests.empty()) {
				handled = m_pendingRequests[0].parser->HandleContinuationResponse();
			}

			if(!handled) {
				// This shouldn't ever happen
				MojLogWarning(m_log, "unhandled continuation response");
			}
		} else {
			// tagged response
			ImapStatusCode status;
			string tag, response;

			try {
				ImapResponseParser::SplitResponse(line, tag, status, response);
			} catch(const MailException& e) {
				MojLogError(m_log, "error parsing response: %s", line.c_str());
				throw;
			}

			if(boost::starts_with(response, "[ALERT]")) {
				MojLogWarning(m_log, "server warning: \"%s\"", response.c_str());
			}

			// Find a pending request with a matching tag
			// Don't call the callback until after this loop, to avoid reentrant SendRequest calls
			std::vector<PendingRequest>::iterator it;
			MojRefCountedPtr<ImapResponseParser> parser;
			for(it = m_pendingRequests.begin(); it != m_pendingRequests.end(); ++it) {
				if(tag == it->tag) {
					parser = it->parser;
					m_pendingRequests.erase(it);
					break;
				}
			}
			
			// Check if we found a matching tag
			if(parser.get()) {
				parser->BaseHandleResponse(status, response);
			} else {
				MojLogError(m_log, "no response handler for tag '%s'", tag.c_str());
			}
		}
	}
}

void ImapRequestManager::RequestLine(const MojRefCountedPtr<ImapResponseParser>& responseParser, int timeoutSeconds)
{
	assert( !m_waitingForLine );
	assert( m_currentParser.get() == NULL || m_currentParser == responseParser );

	WaitForParser(responseParser);
	m_session.GetLineReader()->WaitForLine(m_handleMoreDataSlot, timeoutSeconds);
}

void ImapRequestManager::RequestData(const MojRefCountedPtr<ImapResponseParser>& responseParser, size_t bytes, int timeoutSeconds)
{
	assert( !m_waitingForLine );
	assert( m_currentParser.get() == NULL || m_currentParser == responseParser );

	WaitForParser(responseParser);
	m_session.GetLineReader()->WaitForData(m_handleMoreDataSlot, bytes, timeoutSeconds);
}

// Used to service requests by parsers for additional data
MojErr ImapRequestManager::HandleMoreData()
{
	if(m_currentParser.get()) {
		bool needsMoreData;

		try {
			needsMoreData = m_currentParser->HandleAdditionalData();
		} catch(const exception& e) {
			MojLogError(m_log, "exception in %s: %s", __PRETTY_FUNCTION__, e.what());

			// Generally, we can't recover from this kind of parser error
			// because we don't know the state of the connection.

			// Kill the connection
			m_session.FatalError("exception in ImapRequestManager::HandleMoreData");

			// Kill all pending requests
			ReportException(e);
			return MojErrNone;
		}

		// When we're done, start listening for command responses again
		if(!needsMoreData) {
			ParserFinished(m_currentParser);
		}
	} else {
		MojLogError(m_log, "%s called without a parser", __func__);
	}

	return MojErrNone;
}

void ImapRequestManager::WaitForParser(const MojRefCountedPtr<ImapResponseParser>& parser)
{
	if(m_currentParser.get() == NULL || m_currentParser == parser) {
		m_currentParser = parser;
	} else {
		throw MailException("already have an active parser", __FILE__, __LINE__);
	}
}

void ImapRequestManager::ParserFinished(const MojRefCountedPtr<ImapResponseParser>& parser)
{
	if(m_currentParser == parser) {
		m_currentParser.reset();

		if(!m_pendingRequests.empty() && !m_waitingForLine && !m_handlingResponses) {
			WaitForResponses();
		}
	}
}

void ImapRequestManager::HandleGenericUntaggedResponse(const std::string& line)
{
	try {
		m_untaggedUpdateParser->HandleUntaggedResponse(line);
	} catch(const exception& e) {
		MojLogError(m_log, "caught exception in %s: %s", __PRETTY_FUNCTION__, e.what());
		ReportException(e);
	} catch(...) {
		MojLogError(m_log, "unknown exception in %s", __PRETTY_FUNCTION__);
		ReportException(UNKNOWN_EXCEPTION);
	}
}

void ImapRequestManager::ReportException(const exception& e)
{
	BOOST_FOREACH(const PendingRequest& request, m_pendingRequests) {
		MojRefCountedPtr<ImapResponseParser> parser = request.parser;
		parser->HandleError(e);
	}

	Reset();
}

void ImapRequestManager::CancelRequests()
{
	ReportException( MailException("cancelling requests", __FILE__, __LINE__) );
	Reset();
}

void ImapRequestManager::Reset()
{
	m_pendingRequests.clear();
	m_currentRequestId = 0;
}

void ImapRequestManager::Status(MojObject& status)
{
	MojErr err;
	err = status.put("waitingForLine", m_waitingForLine);
	ErrorToException(err);

	err = status.put("numPendingRequests", (MojInt64) m_pendingRequests.size());
	ErrorToException(err);

	err = status.put("timeout", GetNextTimeout());
	ErrorToException(err);
}
