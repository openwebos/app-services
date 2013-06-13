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

#ifndef IMAPRESPONSEPARSER_H_
#define IMAPRESPONSEPARSER_H_

#include "core/MojSignal.h"
#include "exceptions/Rfc3501ParseException.h"
#include <string>
#include "ImapCoreDefs.h"
#include <boost/exception_ptr.hpp>

class ImapSession;

#define ThrowParseException(msg) throw Rfc3501ParseException(msg, __FILE__, __LINE__)

class ImapResponseParser : public MojSignalHandler
{
public:
	typedef MojSignal<> DoneSignal;
	typedef MojSignal<> ContinuationSignal;
	
	ImapResponseParser(ImapSession& session);
	ImapResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot);
	virtual ~ImapResponseParser();
	
	virtual void CheckStatus();
	virtual ImapStatusCode GetStatus() const { return m_status; }
	virtual const std::string& GetResponseLine() const { return m_responseLine; }
	
	// Handle an untagged response
	virtual bool HandleUntaggedResponse(const std::string& line);

	// Handle a response from the server; OK, NO, or BAD.
	virtual void HandleResponse(ImapStatusCode status, const std::string& response);
	virtual void BaseHandleResponse(ImapStatusCode status, const std::string& line);

	// Handle extra requested data, for literals and malformed multi-line responses
	virtual bool HandleAdditionalData();
	
	virtual void SetContinuationResponseSlot(ContinuationSignal::SlotRef continuationSlot);
	// Handle a response from +
	virtual bool HandleContinuationResponse();

	virtual void HandleError(const std::exception& e);

	/**
	 * Splits a string on the first space.
	 */
	static void SplitOnce(const std::string& line, std::string& word, std::string &rest);

	/**
	 * Splits a response line into tag, status, and the rest of the line.
	 */
	static void SplitResponse(const std::string& line, std::string& tag, ImapStatusCode& responseType, std::string& remaining);

protected:
	virtual void SetResponse(ImapStatusCode status, const std::string& response);
	virtual void Done();
	
	ImapSession&	m_session;
	MojLogger&		m_log;
	
	// Whether the session is waiting for us to finish parsing
	bool			m_sessionWaiting;

	// Exception (if any)
	boost::exception_ptr	m_exception;
	
	ImapStatusCode	m_status;
	std::string		m_responseLine;
	DoneSignal		m_doneSignal;
	ContinuationSignal m_continuationSignal;
};

#endif /*IMAPRESPONSEPARSER_H_*/
