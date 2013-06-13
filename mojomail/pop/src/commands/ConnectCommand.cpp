// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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
#include "client/PopSession.h"
#include "data/PopAccount.h"
#include "exceptions/MailException.h"
#include "PopConfig.h"
#include <iostream>

ConnectCommand::ConnectCommand(PopSession& session)
: PopSessionCommand(session),
  m_connectedSlot(this, &ConnectCommand::Connected),
  m_handleResponseSlot(this, &ConnectCommand::HandleResponse)
{
}

ConnectCommand::~ConnectCommand()
{
}

void ConnectCommand::RunImpl()
{
	try {
		boost::shared_ptr<PopAccount> accnt = m_session.GetAccount();
		std::string hostname = accnt->GetHostName();
		int port = accnt->GetPort();
		std::string encryption = accnt->GetEncryption();
		bool useSSL = encryption == PopAccount::SOCKET_ENCRYPTION_SSL;

		if(!hostname.empty()) {
			MojLogInfo(m_log, "Connecting to %s:%d via ssl=%d", hostname.c_str(), port, useSSL);
			m_connection = SocketConnection::CreateSocket(hostname.c_str(), port,
					useSSL);
			m_connection->Connect(m_connectedSlot);
		} else {
			NetworkFailure(MailError::INTERNAL_ACCOUNT_MISCONFIGURED, MailException("Hostname is empty!", __FILE__, __LINE__));
		}
	} catch (const std::exception& ex) {
		NetworkFailure(MailError::CONNECTION_FAILED, ex);
	}
}

MojErr ConnectCommand::Connected(const std::exception* exc)
{
	try {
		if (!exc) {
			MojLogInfo(m_log, "Connected to server");

			m_session.SetConnection(m_connection);

			// waiting for server to respond with greeting (already connected now)
			m_session.GetLineReader()->WaitForLine(m_handleResponseSlot, PopConfig::READ_TIMEOUT_IN_SECONDS);
		}
		else {
			MojLogInfo(m_log, "Could not connect to server: %s", exc->what());
			NetworkFailure(MailError::CONNECTION_FAILED, *exc);
		}
	} catch (const MailNetworkTimeoutException& nex) {
		NetworkFailure(MailError::CONNECTION_TIMED_OUT, nex);
	} catch (const MailNetworkDisconnectionException& nex) {
		NetworkFailure(MailError::NO_NETWORK, nex);
	} catch (const std::exception& ex) {
		NetworkFailure(MailError::CONNECTION_FAILED, ex);
	} catch (...) {
		NetworkFailure(MailError::CONNECTION_FAILED, MailException("Unknown exception in connecting to POP server", __FILE__, __LINE__));
	}

	return MojErrNone;
}

MojErr ConnectCommand::HandleResponse()
{
	try
	{
		MojLogInfo(m_log, "Received connect response from server");
		std::string line = m_session.GetLineReader()->ReadLine();
		MojLogInfo(m_log, "response: %s", line.c_str());

		if(line.find("+OK") != std::string::npos) {
			m_session.Connected();
			Complete();
		} else {
			char errMsg[320];
			std::string tempLine = line;
			boost::shared_ptr<PopAccount> accnt = m_session.GetAccount();
			if (line.length() > 100) {
				// only use the 100 chars
				tempLine = line.substr(0, 100);
			}

			MojLogError(m_log, "Error connecting to server '%s:%d': %s",
					accnt->GetHostName().c_str(), accnt->GetPort(), errMsg);
			MailException err(errMsg, __FILE__, __LINE__);
			NetworkFailure(MailError::CONNECTION_FAILED, err);
		}
	}catch(const std::exception& e) {
		MojLogError(m_log, "Error connecting to server: %s", e.what());
		NetworkFailure(MailError::CONNECTION_FAILED, e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Error connecting to server ", __FILE__, __LINE__);
		NetworkFailure(MailError::CONNECTION_FAILED, exc);
	}

	return MojErrNone;
}
