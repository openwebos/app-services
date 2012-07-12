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

#ifndef TLSCOMMAND_H_
#define TLSCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "protocol/CapabilityResponseParser.h"
#include "network/SocketConnection.h"

class TLSCommand : public ImapSessionCommand
{
public:
	TLSCommand(ImapSession& session);
	virtual ~TLSCommand();

	void RunImpl();
	MojErr StartTLSResponse();

	void Failure(const std::exception& e);

protected:
	void NegotiateTLS();
	MojErr TLSNegotiationCompleted(const std::exception* exc);

	MojRefCountedPtr<CapabilityResponseParser>	m_responseParser;

	ImapResponseParser::DoneSignal::Slot<TLSCommand>	m_startTLSResponseSlot;
	SocketConnection::TLSReadySignal::Slot<TLSCommand> m_tlsCompletedSlot;
};

#endif /* TLSCOMMAND_H_ */
