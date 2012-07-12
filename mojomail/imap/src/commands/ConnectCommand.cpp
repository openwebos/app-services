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
#include "client/ImapSession.h"
#include "data/ImapLoginSettings.h"
#include "ImapPrivate.h"
#include "ImapConfig.h"

ConnectCommand::ConnectCommand(ImapSession& session, const std::string& bindAddress)
: ImapSessionCommand(session),
  m_bindAddress(bindAddress),
  m_connectedSlot(this, &ConnectCommand::Connected)
{
}

ConnectCommand::~ConnectCommand()
{
}

void ConnectCommand::RunImpl()
{
	CommandTraceFunction();

	try {
		const ImapLoginSettings& loginSettings = m_session.GetLoginSettings();

		std::string hostname = loginSettings.GetHostname();
		int port = loginSettings.GetPort();
		ImapLoginSettings::EncryptionType encryption = loginSettings.GetEncryption();

		if(hostname.empty() || port <= 0) {
			throw MailException("empty hostname or port", __FILE__, __LINE__);
		}

		MojLogInfo(m_log, "connecting to %s:%d%s on interface %s",
				hostname.c_str(), port, (encryption == ImapLoginSettings::Encrypt_SSL) ? " with SSL" : "",
				!m_bindAddress.empty() ? m_bindAddress.c_str() : "0.0.0.0");

		m_connection = SocketConnection::CreateSocket(hostname.c_str(), port,
				(encryption == ImapLoginSettings::Encrypt_SSL), m_bindAddress);

		int connectTimeout = ImapConfig::GetConfig().GetConnectTimeout();

		if(connectTimeout) {
			m_connection->SetConnectTimeout(connectTimeout);
		}

		m_connection->Connect(m_connectedSlot);
	} catch(const exception& e) {
		ConnectFailure(e);
	}
}

MojErr ConnectCommand::Connected(const std::exception* exc)
{
	CommandTraceFunction();

	if(exc == NULL) {
		MojLogInfo(m_log, "connected to server");
		m_session.Connected(m_connection);

		Complete();
	} else {
		ConnectFailure(*exc);
	}
	
	return MojErrNone;
}

void ConnectCommand::ConnectFailure(const exception& e)
{
	CommandTraceFunction();

	MojLogError(m_log, "error connecting to server: %s", e.what());

	if(m_connection.get()) {
		m_connection->Shutdown();
	}

	m_session.ConnectFailure(e);
	Failure(e);
}

std::string ConnectCommand::Describe() const
{
	if(m_session.HasClient()) {
		return ImapSessionCommand::Describe() + " [accountId=" + m_session.GetClient()->GetAccountIdString() + "]";
	} else {
		return ImapSessionCommand::Describe();
	}
}
