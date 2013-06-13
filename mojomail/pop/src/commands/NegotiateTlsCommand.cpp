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

#include "commands/NegotiateTlsCommand.h"

NegotiateTlsCommand::NegotiateTlsCommand(PopSession& session)
: PopSessionCommand(session),
  m_tlsNegotiatedSlot(this, &NegotiateTlsCommand::TlsNegotiated)
{

}

NegotiateTlsCommand::~NegotiateTlsCommand()
{

}

void NegotiateTlsCommand::RunImpl()
{
	CommandTraceFunction();
	try{
		m_session.GetConnection()->NegotiateTLS(m_tlsNegotiatedSlot);
	}
	catch (const std::exception& e) {
		MojLogError(m_log, "Exception in TLS negotiation: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in TLS negotiation");
		Failure(MailException("Unknown exception in TLS negotiation", __FILE__, __LINE__));
	}
}

MojErr NegotiateTlsCommand::TlsNegotiated(const std::exception* exc)
{
	CommandTraceFunction();
	if(!exc) {
		MojLogInfo(m_log, "negotiation Completed");
		m_session.TlsNegotiated();
	} else {
		MojLogError(m_log, "error connecting to server");
		m_session.TlsFailed();
	}

	Complete();
	return MojErrNone;
}
