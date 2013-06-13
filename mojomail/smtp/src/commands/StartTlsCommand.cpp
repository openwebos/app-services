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

#include "commands/StartTlsCommand.h"
#include "exceptions/ExceptionUtils.h"

const char* const StartTlsCommand::COMMAND_STRING		= "STARTTLS";

StartTlsCommand::StartTlsCommand(SmtpSession& session)
: SmtpProtocolCommand(session),
  m_tlsNegotiatedSlot(this, &StartTlsCommand::TlsNegotiated)
{

}

StartTlsCommand::~StartTlsCommand()
{
}

void StartTlsCommand::RunImpl()
{
	MojLogInfo(m_log, "asking for TLS to start");

	SendCommand(COMMAND_STRING);
}

MojErr StartTlsCommand::HandleResponse(const std::string& line)
{
	CommandTraceFunction();

	if (m_status == Status_Ok) {
		MojLogInfo(m_log, "Negotiating TLS");
		m_session.GetConnection()->NegotiateTLS(m_tlsNegotiatedSlot);
	} else {
		MojLogInfo(m_log, "TLS command failed");
		SmtpSession::SmtpError error = GetStandardError();
		error.internalError = "TLS command failed";
		m_session.TlsFailure(error);
		Complete();
	}

	return MojErrNone;
}

MojErr StartTlsCommand::TlsNegotiated(const std::exception* exc)
{
	CommandTraceFunction();

	MojLogInfo(m_log, "TLS negotiated");

	if(!exc) {
		MojLogInfo(m_log, "negotiation Completed");
		m_session.TlsSuccess();
	} else {
		MojLogError(m_log, "error connecting to server: %s", exc->what());

		SmtpSession::SmtpError error = ExceptionUtils::GetErrorInfo(*exc);
		error.errorOnAccount = true;
		error.internalError = std::string("TLS negotiation failed: ") + exc->what();
		
		m_session.TlsFailure(error);
	}

	Complete();
	return MojErrNone;
}

