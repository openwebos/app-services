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

#include "commands/TLSCommand.h"
#include "client/ImapSession.h"
#include "data/ImapAccount.h"

using namespace std;
TLSCommand::TLSCommand(ImapSession& session)
: ImapSessionCommand(session),
  m_startTLSResponseSlot(this, &TLSCommand::StartTLSResponse),
  m_tlsCompletedSlot(this, &TLSCommand::TLSNegotiationCompleted)
{
}

TLSCommand::~TLSCommand()
{
}

void TLSCommand::RunImpl()
{
	CommandTraceFunction();

	// Reset capabilities
	m_session.GetCapabilities().SetValid(false);

	MojLogInfo(m_log, "sending STARTTLS command");
	m_responseParser.reset(new CapabilityResponseParser(m_session, m_startTLSResponseSlot));

	std::string command = "STARTTLS";

	m_session.SendRequest(command, m_responseParser, false);
}

MojErr TLSCommand::StartTLSResponse()
{
	CommandTraceFunction();

	try {
		ImapStatusCode status = m_responseParser->GetStatus();
		string line = m_responseParser->GetResponseLine();

		if (status == OK) {
			// 0 == TLS negotiation started
			// line reads "STARTTLS Ok."
			NegotiateTLS();
		} else if(status == BAD || status == NO) {
			// TODO report TLS not supported
			m_session.TLSFailure(MailException("TLS not supported", __FILE__, __LINE__));
		} else {
			m_responseParser->CheckStatus();
		}

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void TLSCommand::NegotiateTLS()
{
	MojLogInfo(m_log, "negotiating TLS");

	m_session.GetConnection()->NegotiateTLS(m_tlsCompletedSlot);
}

MojErr TLSCommand::TLSNegotiationCompleted(const std::exception* exc)
{
	CommandTraceFunction();

	if(!exc) {
		MojLogInfo(m_log, "negotiation Completed");
		m_session.TLSReady();
	} else {
		MojLogError(m_log, "error negotiating TLS: %s", exc->what());
		m_session.TLSFailure(*exc);
	}

	Complete();
	return MojErrNone;
}

void TLSCommand::Failure(const std::exception& e)
{
	ImapSessionCommand::Failure(e);

	m_session.TLSFailure(e);
}
