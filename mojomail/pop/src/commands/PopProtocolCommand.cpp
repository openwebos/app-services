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

#include <iostream>
#include <boost/regex.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "commands/PopProtocolCommand.h"
#include "commands/PopProtocolCommandErrorResponse.h"
#include "stream/BaseOutputStream.h"
#include "PopConfig.h"

using namespace boost;
using namespace std;

const char* const PopProtocolCommand::STATUS_STRING_OK 		= "+OK";
const char* const PopProtocolCommand::STATUS_STRING_ERR 	= "-ERR";
const char* const PopProtocolCommand::CRLF					= "\r\n";

PopProtocolCommand::PopProtocolCommand(PopSession& session, Priority priority)
: PopSessionCommand(session, priority),
  m_handleResponseSlot(this, &PopProtocolCommand::ReceiveResponse),
  m_includesCRLF(false),
  m_status(Status_Err),
  m_errorCode(MailError::NONE)
{

}

PopProtocolCommand::~PopProtocolCommand()
{

}

void PopProtocolCommand::SendCommand(const std::string& request)
{
	try
	{
		m_requestStr = request;   // 'm_requestStr' will be used to report error
		std::string reqStr = request + CRLF;
		OutputStreamPtr outputStreamPtr = m_session.GetOutputStream();
		outputStreamPtr->Write(reqStr.c_str());

		MojLogDebug(m_log, "Sent command: '%s'", request.c_str());
		m_session.GetLineReader()->WaitForLine(m_handleResponseSlot, PopConfig::READ_TIMEOUT_IN_SECONDS);
	} catch (const MailNetworkDisconnectionException& nex) {
		NetworkFailure(MailError::NO_NETWORK, nex);
	} catch (const std::exception& ex) {
		MojLogError(m_log, "Exception in send command: '%s'", ex.what());
		m_errorCode = MailError::CONNECTION_FAILED;
		NetworkFailure(m_errorCode, ex);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in sending command");
		m_errorCode = MailError::CONNECTION_FAILED;
		NetworkFailure(m_errorCode, MailException("Unknown exception in sending command", __FILE__, __LINE__));
	}
}

MojErr PopProtocolCommand::ReceiveResponse()
{
	try{
		m_responseFirstLine = m_session.GetLineReader()->ReadLine(m_includesCRLF);
	} catch (const MailNetworkTimeoutException& nex) {
		m_errorCode = MailError::CONNECTION_TIMED_OUT;
		NetworkFailure(m_errorCode, nex);
	} catch (const MailNetworkDisconnectionException& nex) {
		m_errorCode = MailError::NO_NETWORK;
		NetworkFailure(m_errorCode, nex);
	} catch (const std::exception& ex) {
		MojLogError(m_log, "Exception in receiving pop response: '%s'", ex.what());
		m_errorCode = MailError::CONNECTION_FAILED;
		NetworkFailure(m_errorCode, ex);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in receiving pop response");
		m_errorCode = MailError::CONNECTION_FAILED;
		NetworkFailure(m_errorCode, MailException("Unknown exception in receiving pop response", __FILE__, __LINE__));
	}

	if (m_errorCode == MailError::CONNECTION_FAILED
			|| m_errorCode == MailError::CONNECTION_TIMED_OUT
			|| m_errorCode == MailError::NO_NETWORK) {
		return MojErrInternal;
	}

	try {
		MojLogDebug(m_log, "Response %s", m_responseFirstLine.c_str());
		ParseResponseFirstLine();

		if (m_errorCode == MailError::BAD_USERNAME_OR_PASSWORD
				|| m_errorCode == MailError::ACCOUNT_LOCKED
				|| m_errorCode == MailError::ACCOUNT_UNAVAILABLE
				|| m_errorCode == MailError::ACCOUNT_UNKNOWN_AUTH_ERROR
				|| m_errorCode == MailError::ACCOUNT_WEB_LOGIN_REQUIRED) {
			m_session.LoginFailure(m_errorCode, m_serverMessage);
			Failure(MailException("Login failure", __FILE__, __LINE__));

			return MojErrInternal;
		} else {
			HandleResponse(m_responseFirstLine);
		}
	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in processing pop response: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in processing pop response");
		Failure(MailException("Unknown exception in processing pop response", __FILE__, __LINE__));
	}
	return MojErrNone;
}

void PopProtocolCommand::ParseResponseFirstLine()
{
	try{
		// initialize status string with the response first line
		if (m_responseFirstLine.find(STATUS_STRING_OK) != string::npos) {
			m_status = Status_Ok;
			m_serverMessage = m_responseFirstLine.substr(strlen(STATUS_STRING_OK));
		} else if (m_responseFirstLine.find(STATUS_STRING_ERR) != string::npos) {
			m_status = Status_Err;
			m_serverMessage = m_responseFirstLine.substr(strlen(STATUS_STRING_ERR));
			AnalyzeCommandResponse(m_serverMessage);
		} else {
			m_status = Status_Err;
			m_serverMessage = m_responseFirstLine;
		}
	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in parsing the first line: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception parsing the first line");
		Failure(MailException("Unknown exception parsing the first line", __FILE__, __LINE__));
	}
	//MojLogInfo(m_log, "Status: %i, server message: %s", m_status, m_serverMessage.c_str());
}

void PopProtocolCommand::AnalyzeCommandResponse(const std::string& serverMessage)
{
	if (!serverMessage.empty() && IsHotmail()) {
		AnalyzeHotmailResponse(serverMessage);
	}
}

bool PopProtocolCommand::IsHotmail() {
	std::string host = m_session.GetAccount()->GetHostName();
	return host.find("live.com") != std::string::npos;
}

void PopProtocolCommand::AnalyzeHotmailResponse(const std::string& serverMessage)
{
	// Don't want to catch the exception in following codes.  Just let it be thrown and
	// caught as app error so that we can be notified in RDX report and fix it accordingly.
	MojLogError(m_log, "server msg:%s", serverMessage.data());
	if (boost::icontains(serverMessage, PopProtocolCommandErrorResponse::HOTMAIL_LOGIN_TOO_FREQUENT)||
		boost::icontains(serverMessage, PopProtocolCommandErrorResponse::HOTMAIL_LOGIN_LIMIT_EXCEED)||
		boost::icontains(serverMessage, PopProtocolCommandErrorResponse::HOTMAIL_MAILBOX_NOT_OPENED)) {
		MojLogError(m_log, "ACCOUNT_UNAVAILABLE");
		m_errorCode = MailError::ACCOUNT_UNAVAILABLE;
		return;
	}

	if (boost::icontains(serverMessage, PopProtocolCommandErrorResponse::HOTMAIL_MAILBOX_NOT_OPENED)) {
		m_errorCode = MailError::ACCOUNT_UNAVAILABLE;
		return;
	}

	if (boost::icontains(serverMessage, PopProtocolCommandErrorResponse::HOTMAIL_WEB_LOGIN_REQUIRED)) {
		m_errorCode = MailError::ACCOUNT_WEB_LOGIN_REQUIRED;
		return;
	}
}

void PopProtocolCommand::AnalyzeMailException(const MailException& ex)
{
	MojRefCountedPtr<SocketConnection> connection = m_session.GetConnection();
	if (connection.get() && connection->GetErrorInfo().errorCode != MailError::NONE) {
		NetworkFailure(MailError::CONNECTION_FAILED, ex);
	} else {
		MojLogError(m_log, "Mail exception in receiving multilines response: '%s'", ex.what());
		Failure(ex);
	}
}
