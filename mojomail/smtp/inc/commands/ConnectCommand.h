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

#ifndef CONNECTCOMMAND_H_
#define CONNECTCOMMAND_H_

#include "network/SocketConnection.h"
#include "commands/SmtpProtocolCommand.h"
#include "core/MojObject.h"
#include "stream/LineReader.h"

/**
 * A command class that establishes a connection to the remote SMTP server.
 */
class ConnectCommand : public SmtpProtocolCommand
{
public:
	ConnectCommand(SmtpSession& session);
	virtual ~ConnectCommand();

	MojErr Connected(const std::exception* exc);
	MojErr HandleResponse(const std::string& line);

protected:
	virtual void RunImpl();

	void WaitForGreeting();

	SocketConnection::ConnectedSignal::Slot<ConnectCommand>	m_connectedSlot;

private:
	MojRefCountedPtr<SocketConnection> m_connection;
};

#endif /* CONNECTCOMMAND_H_ */
