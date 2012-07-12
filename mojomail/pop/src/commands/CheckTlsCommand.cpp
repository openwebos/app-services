// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "commands/CheckTlsCommand.h"
#include "exceptions/MailException.h"

const char* const CheckTlsCommand::COMMAND_STRING		= "STLS";

CheckTlsCommand::CheckTlsCommand(PopSession& session)
: PopProtocolCommand(session)
{

}

CheckTlsCommand::~CheckTlsCommand()
{

}

void CheckTlsCommand::RunImpl()
{
	if (!m_session.HasNetworkError()) {
		SendCommand(COMMAND_STRING);
	} else {
		MojLogInfo(m_log, "Abort check TLS command due to network");
		Complete();
	}
}

MojErr CheckTlsCommand::HandleResponse(const std::string& line)
{
	try
	{
		if (m_status == Status_Ok) {
			m_session.TlsSupported();
			Complete();
		} else {
			char errMsg[320];
			std::string tempLine = line;
			if (line.length() > 100) {
				// only use the 100 chars
				tempLine = line.substr(0, 100);
			}

			MojLogError(m_log, "TLS not supported: %s", line.c_str());
			MailException err(errMsg, __FILE__, __LINE__);
			NetworkFailure(MailError::CONFIG_NO_SSL, err);
		}
	} catch(const std::exception& e) {
		MojLogError(m_log, "TLS not supported: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("TLS not supported", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}
