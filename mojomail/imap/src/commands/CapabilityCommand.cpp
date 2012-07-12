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

#include "commands/CapabilityCommand.h"
#include "protocol/CapabilityResponseParser.h"
#include "client/ImapSession.h"

CapabilityCommand::CapabilityCommand(ImapSession& session)
: ImapSessionCommand(session),
  m_responseSlot(this, &CapabilityCommand::Response)
{
}

CapabilityCommand::~CapabilityCommand()
{
}

void CapabilityCommand::RunImpl()
{
	m_responseParser.reset(new CapabilityResponseParser(m_session, m_responseSlot));
	m_session.SendRequest("CAPABILITY", m_responseParser);
}

MojErr CapabilityCommand::Response()
{
	try {
		m_responseParser->CheckStatus();

		m_session.CapabilityComplete();
		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}
