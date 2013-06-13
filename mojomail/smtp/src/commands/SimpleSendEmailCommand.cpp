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

#include "commands/SimpleSendEmailCommand.h"
#include <iostream>

const char* const SimpleSendEmailCommand::FROM_COMMAND_STRING		= "MAIL FROM:";
const char* const SimpleSendEmailCommand::TO_COMMAND_STRING		= "RCPT TO:";
const char* const SimpleSendEmailCommand::DATA_COMMAND_STRING		= "DATA";

SimpleSendEmailCommand::SimpleSendEmailCommand(SmtpSession& session, const std::string& fromAddress, const std::vector<std::string>& toAddress, const std::string& data)
: SmtpProtocolCommand(session),
  m_state(0),
  m_toIdx(0),
  m_fromAddress(fromAddress),
  m_toAddress(toAddress),
  m_data(data)
{

}

SimpleSendEmailCommand::~SimpleSendEmailCommand()
{

}


// Crude implementation for testing protocol flow, not currently connected to email DB
void SimpleSendEmailCommand::RunImpl()
{
	if (m_state == 0) {
		MojLogInfo(m_log, "Sending MAIL FROM command");
		MojLogDebug(m_log, "From address is %s\n", m_fromAddress.c_str());

		std::string userCommand = std::string(FROM_COMMAND_STRING) + " " + m_fromAddress;
		SendCommand(userCommand);
	} else if (m_state == 1) {
		MojLogInfo(m_log, "Sending RCPT TO command");

		std::string userCommand = std::string(TO_COMMAND_STRING) + " " + m_toAddress[m_toIdx];
		SendCommand(userCommand);
	} else if (m_state == 2) {
		MojLogInfo(m_log, "Sending DATA command");

		std::string userCommand = DATA_COMMAND_STRING;
		SendCommand(userCommand);
	} else if (m_state == 3) {
		MojLogInfo(m_log, "Sending BODY");

		std::string userCommand = DATA_COMMAND_STRING;
		SendCommand(m_data + "\r\n.");
	}
}

MojErr SimpleSendEmailCommand::HandleResponse(const std::string& line)
{
	if (m_state == 0) {
		MojLogInfo(m_log, "MAIL FROM command response");

		if (m_status == Status_Ok) {
			MojLogInfo(m_log, "MAIL FROM command +OK");
			m_toIdx = 0;
			m_state = 1;
			RunImpl();
			
		} else {
			MojLogError(m_log, "MAIL FROM command -ERR: %s", line.c_str());
			m_session.SendFailure("Error setting return address");
			Complete();
		}
	} else if (m_state == 1) {
		MojLogInfo(m_log, "RCPT TO command response");

		if (m_status == Status_Ok) {
			m_toIdx++;
			if (m_toIdx < m_toAddress.size()) {
				MojLogInfo(m_log, "RCPT TO command +OK, more to come");
				m_state = 1;
			} else {
				MojLogInfo(m_log, "RCPT TO command +OK, done");
				m_state = 2;
			}
			RunImpl();
			
		} else {
			MojLogError(m_log, "RCPT TO command -ERR: %s", line.c_str());
			m_session.SendFailure("Error setting recipient address");
			Complete();
		}
	} else if (m_state == 2) {
		MojLogInfo(m_log, "DATA command response");

		if (m_status == Status_Ok) {
			MojLogInfo(m_log, "DATA command +OK");
			m_state = 3;
			RunImpl();
			
		} else {
			MojLogError(m_log, "DATA command -ERR: %s", line.c_str());
			m_session.SendFailure("Error starting mail body\n");
			Complete();
		}
	} else if (m_state == 3) {
		MojLogInfo(m_log, "BODY command response");

		if (m_status == Status_Ok) {
			MojLogInfo(m_log, "BODY command +OK");
			m_session.SendSuccess();
			Complete();
		} else {
			MojLogError(m_log, "BODY command -ERR: %s", line.c_str());
			m_session.SendFailure("Error sending mail body\n");
			Complete();
		}
	}
	return MojErrNone;
}
