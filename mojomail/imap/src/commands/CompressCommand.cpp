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

#include "commands/CompressCommand.h"
#include "client/ImapSession.h"
#include "protocol/ImapResponseParser.h"

CompressCommand::CompressCommand(ImapSession& session)
: ImapSessionCommand(session),
  m_responseSlot(this, &CompressCommand::CompressResponse)
{
}

CompressCommand::~CompressCommand()
{
}

void CompressCommand::RunImpl()
{
	CommandTraceFunction();

	m_responseParser.reset(new ImapResponseParser(m_session, m_responseSlot));
	m_session.SendRequest("COMPRESS DEFLATE", m_responseParser);
}

MojErr CompressCommand::CompressResponse()
{
	CommandTraceFunction();

	try {
		ImapStatusCode status = m_responseParser->GetStatus();

		if(status == OK) {
			// enable compression now
			m_session.CompressComplete(true);
		} else if(status == BAD || status == NO) {
			// don't enable compression
			m_session.CompressComplete(false);
		} else {
			m_responseParser->CheckStatus();
		}

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void CompressCommand::Failure(const std::exception& e)
{
	// We're in trouble
	m_session.FatalError(e.what());

	ImapSessionCommand::Failure(e);
}

void CompressCommand::Cleanup()
{
	m_responseParser.reset();

	m_responseSlot.cancel();

	ImapSessionCommand::Cleanup();
}
