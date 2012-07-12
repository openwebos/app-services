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

#include <iostream>
#include "commands/SmtpProtocolCommand.h"
#include "stream/BaseOutputStream.h"
#include "exceptions/MailException.h"

using namespace std;

// constants for SMTP response codes should go here

SmtpProtocolCommand::SmtpProtocolCommand(SmtpSession& session, Priority priority)
: SmtpSessionCommand(session, priority),
  m_handleResponseSlot(this, &SmtpProtocolCommand::ReceiveResponse),
  m_serverMessage(""),
  m_inResponse(false),
  m_status(Status_Err)
{

}

SmtpProtocolCommand::~SmtpProtocolCommand()
{

}

SmtpSession::SmtpError SmtpProtocolCommand::GetStandardError()
{
	SmtpSession::SmtpError error;
	
	error.errorCode = MailError::NONE;
	error.errorText = "";
	error.internalError = "";
	error.errorOnAccount = false;
	error.errorOnEmail = false;
	
	if (m_statusCode == 497) {		// internal code, indicating timeout
		error.errorCode = MailError::CONNECTION_TIMED_OUT;
		error.errorOnAccount = true;
	} else if (m_statusCode == 498) {		// internal code, indicating socket closed
		error.errorCode = MailError::CONNECTION_FAILED;
		error.errorOnAccount = true;
	} else if (m_statusCode == 499) {	// internal code, indicating other network issue
		error.errorCode = MailError::CONNECTION_FAILED;
		error.errorOnAccount = true;
	} else if (m_statusCode == 421) {	//service shutting down
		error.errorCode = MailError::SMTP_SHUTTING_DOWN;
		error.errorText = m_responseLine;
		error.errorOnAccount = true;
	} else if (m_statusCode == 550) { // 'I'm sorry, Dave. I'm afraid I can't do that. '
		error.errorCode = MailError::SMTP_UNKNOWN_ERROR; // might be overridden
		error.errorText = m_responseLine;
		error.errorOnEmail = true;
	} else if (m_statusCode == 551) { // 'user not local'
		error.errorCode = MailError::BAD_RECIPIENTS;
		error.errorText = m_responseLine;
		error.errorOnEmail = true;
	} else if (m_statusCode == 552) {
		error.errorCode = MailError::EMAIL_SIZE_EXCEEDED;
		error.errorText = m_responseLine;
		error.errorOnEmail = true;
	} else if (m_statusCode == 530 || m_statusCode == 534 || m_statusCode == 535) {
		error.errorCode = MailError::BAD_USERNAME_OR_PASSWORD;
		error.errorText = m_responseLine;
		error.errorOnAccount = true;
	} else if (m_statusCode >= 400 && m_statusCode <= 499) {
		error.errorCode = MailError::ACCOUNT_UNAVAILABLE;
		error.errorText = m_responseLine;
		error.errorOnAccount = true; // presume temporary error
	} else if (m_statusCode >= 500 && m_statusCode <= 599) {
		error.errorCode = MailError::SMTP_UNKNOWN_ERROR;
		error.errorText = m_responseLine;
		error.errorOnEmail= true; // presume permanent error
	} else {
		// odd, doesn't fall into expected categories, treat as 
		error.errorCode = MailError::SMTP_UNKNOWN_ERROR;
		error.errorText = m_responseLine;
		error.errorOnEmail = true; // safest assumption
	}
	
	return error;
}

// NOTE: May need a SendCommand() with a input response slots.
void SmtpProtocolCommand::SendCommand(const std::string& request)
{
    SendCommand(request, SmtpSession::TIMEOUT_GENERAL);
}

void SmtpProtocolCommand::SendCommand(const std::string& request, int timeout)
{
	if (m_inResponse) {
		MojLogDebug(m_log, "Attempted to send command %s while a command was being received\n", request.c_str());
		MojLogInfo(m_log, "Attempted to send command while a command was being received\n");
		return;
	}
    
	std::string reqStr = request + "\r\n";

	try {
		OutputStreamPtr outputStreamPtr = m_session.GetOutputStream();
		outputStreamPtr->Write(reqStr.c_str());

		m_firstResponse = true;
		m_responseLineNumber = 0;
		m_inResponse = true;

		m_session.GetLineReader()->WaitForLine(m_handleResponseSlot, timeout);
	} catch(const std::exception& e) {
		MojLogWarning(m_log, "exception in %s::SendCommand: %s", GetClassName().c_str(), e.what());
		m_handleResponseSlot.cancel();

		ReceiveResponse();
	} catch(...) {
		MojLogWarning(m_log, "exception in %s::SendCommand: unknown exception", GetClassName().c_str());
		m_handleResponseSlot.cancel();

		ReceiveResponse();
	}
}

MojErr SmtpProtocolCommand::ReceiveResponse()
{
	try {
		m_responseLine = m_session.GetLineReader()->ReadLine();
	} catch (const MailNetworkTimeoutException & e) {
		m_responseLine = "497 Client error: socket timeout";
	} catch (const MailNetworkDisconnectionException & e) {
		m_responseLine = "498 Client error: socket disconnection";
	} catch (const std::exception& e){
		MojLogInfo(m_log, "Got exception in ReceiveResponse: %s", e.what());
		m_responseLine = std::string("499 Client error: ") + e.what() ;
	} catch (...){
		MojLogInfo(m_log, "Got unknown exception in ReceiveResponse");
		m_responseLine = std::string("499 Client error: unknown exception") ;
	}

	MojLogDebug(m_log, "Response: %s", m_responseLine.c_str());
	MojLogInfo(m_log, "Response received");
	
	ParseResponseEachLine();

	if (m_lastResponse)
        m_inResponse = false;
	
	// cache response parameters, in case response action sends a new message and
	// wipes these values.
	std::string saveServerMessage = m_serverMessage;
	int saveResponseLineNumber = m_responseLineNumber;
	bool saveLastResponse = m_lastResponse;
	
	HandleMultilineResponse(saveServerMessage, saveResponseLineNumber, saveLastResponse);

	if (saveLastResponse) {
		HandleResponse(saveServerMessage);
	} else {
	    	// For multi-line response, continue waiting for another line
		m_firstResponse = false;
		m_responseLineNumber++;
		m_session.GetLineReader()->WaitForLine(m_handleResponseSlot);
	}

	return MojErrNone;
}

void SmtpProtocolCommand::ParseResponseEachLine()
{
	// invoked for each response line. The server is expected to provide the same status code
	// for each response line, so it is OK to recompute the status codes for each line.

	// initialize status string with the response line
	string statusStr = m_responseLine;

	m_lastResponse = true;
	
	// Look for dash indicating multi-line response
	if (statusStr.length() >= 4 && statusStr[3] == '-')
		m_lastResponse = false;

	// assumes C_LOCALE -- only accept status code if it is a well-formed triple
	if (statusStr.length() >= 3 && isdigit(statusStr[0]) && isdigit(statusStr[1]) && isdigit(statusStr[2])) {
		m_statusCode = atoi(statusStr.c_str());
	} else {
		m_statusCode = 0;
	}
	
	// servers might inappropriately not place a space after the status code
	if (statusStr.length() > 4) {
		m_serverMessage = m_responseLine.substr(4);
	} else {
		m_serverMessage = "";
	}

	// default processing of status codes into basic status
	switch ((m_statusCode / 100)) {
	case 1:
	case 2:
	case 3:
		m_status = Status_Ok;
		break;
	case 4:
	case 5:
	default:
		m_status = Status_Err;
	}

	//MojLogInfo(m_log, "Status code: %d, Status: %i, line number: %d, last: %d, server message: %s", m_statusCode, m_status, m_responseLineNumber, m_lastResponse ? 1 : 0, m_serverMessage.c_str());

	if (m_status == Status_Err) {
		MojLogError(m_log, "Status code (error): %d, Status: %i, line number: %d, last: %d", m_statusCode, m_status, m_responseLineNumber, m_lastResponse ? 1 : 0);
	} else {
		MojLogInfo(m_log, "Status code (ok): %d, Status: %i, line number: %d, last: %d", m_statusCode, m_status, m_responseLineNumber, m_lastResponse ? 1 : 0);
	}
}

// Stub virtual base implementation. We don't use a pure virtual as the descendant might override either of
// these routines.
MojErr SmtpProtocolCommand::HandleMultilineResponse(const std::string& line, int lineNumber, bool lastLine)
{
    return MojErrNone;
}

// Stub virtual base implementation. We don't use a pure virtual as the descendant might override either of
// these routines.
MojErr SmtpProtocolCommand::HandleResponse(const std::string& line)
{
    return MojErrNone;
}
