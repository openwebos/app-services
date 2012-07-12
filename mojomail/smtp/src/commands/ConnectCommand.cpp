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

#include "commands/ConnectCommand.h"
#include "data/SmtpAccount.h"
#include "SmtpClient.h"
#include <iostream>
#include "exceptions/ExceptionUtils.h"

ConnectCommand::ConnectCommand(SmtpSession& session)
: SmtpProtocolCommand(session),
  m_connectedSlot(this, &ConnectCommand::Connected)
{
}

ConnectCommand::~ConnectCommand()
{
}

void ConnectCommand::RunImpl()
{
	boost::shared_ptr<SmtpAccount> accnt = m_session.GetAccount();
	std::string hostname = accnt->GetHostname();
	int port = accnt->GetPort();
	bool useSSL = accnt->NeedsSSL();

	MojLogInfo(m_log, "SmtpSession %p Connecting via ssl=%d", &m_session, useSSL);
	// FIXME: Utilize SmtpSession::TIMEOUT_CONNECTION

	try {
		m_connection = SocketConnection::CreateSocket(hostname.c_str(), port, useSSL);
		m_connection->Connect(m_connectedSlot);
	} catch(const std::exception& e) {
		MojLogError(m_log, "exception trying to connect: %s", e.what());

		SmtpSession::SmtpError error;
		error.errorCode = MailError::CONNECTION_FAILED;
		error.errorOnAccount = true;
		error.internalError = std::string("socket connection failure: ") + e.what();
		m_session.ConnectFailure(error);
	} catch(...) {
		MojLogError(m_log, "exception trying to connect: unknown exception");

		SmtpSession::SmtpError error;
		error.errorCode = MailError::CONNECTION_FAILED;
		error.errorOnAccount = true;
		error.internalError = "socket connection failure: unknown exception";
		m_session.ConnectFailure(error);
	}
}

MojErr ConnectCommand::Connected(const std::exception* exc)
{
	CommandTraceFunction();

	if (!exc) {
		MojLogInfo(m_log, "SmtpSession %p Connected to server", &m_session);

		m_session.SetConnection(m_connection);

		WaitForGreeting();
	} else {
		MojLogInfo(m_log, "SmtpSession %p Could not connect to server: %s", &m_session, exc->what());

		SmtpSession::SmtpError error = ExceptionUtils::GetErrorInfo(*exc);
		error.errorOnAccount = true;
		error.internalError = "socket connection failure";
		m_session.ConnectFailure(error);
		Complete();
	}

	return MojErrNone;
}

void ConnectCommand::WaitForGreeting()
{
	CommandTraceFunction();

	// waiting for server to respond with greeting (already
	// connected now), using the SmtpProtocolCommand machinery.
	try {
		m_session.GetLineReader()->WaitForLine(m_handleResponseSlot, SmtpSession::TIMEOUT_GREETING);
	} catch(const std::exception& e) {
		MojLogError(m_log, "error reading greeting: %s", e.what());

		m_handleResponseSlot.cancel();
		ReceiveResponse();
	}
}

MojErr ConnectCommand::HandleResponse(const std::string& line)
{
	CommandTraceFunction();

	MojLogInfo(m_log, "SmtpSession %p Received connect greeting from server", &m_session);
	
	if (m_statusCode == 220)
		m_status = Status_Ok;
	else
		m_status = Status_Err;
	
	if (m_status == Status_Ok) {
		m_session.ConnectSuccess();
	} else {
		MojLogError(m_log, "SmtpSession %p Error connecting to server", &m_session);
		SmtpSession::SmtpError error = GetStandardError();
		error.internalError = "invalid server greeting";
		m_session.ConnectFailure(error);
	}

	Complete();
	return MojErrNone;
}
