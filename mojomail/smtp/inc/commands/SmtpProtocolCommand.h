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

#ifndef SMTPPROTOCOLCOMMAND_H_
#define SMTPPROTOCOLCOMMAND_H_

#include "commands/SmtpSessionCommand.h"
#include "SmtpClient.h"
#include "stream/LineReader.h"

class SmtpProtocolCommand : public SmtpSessionCommand
{
public:

	enum StatusCode {
		Status_Ok,
		Status_Err
	};
	SmtpProtocolCommand(SmtpSession& session, Priority priority = NormalPriority);
	virtual ~SmtpProtocolCommand();

	// Invoked for all lines of response
	virtual MojErr HandleMultilineResponse(const std::string& line, int lineNumber, bool lastLine);
	// Invoked only for last line of response
	virtual MojErr HandleResponse(const std::string& line);

protected:
	void 			SendCommand(const std::string& request, int timeout);
	void 			SendCommand(const std::string& request);
	MojErr	 		ReceiveResponse();
	virtual void 	ParseResponseEachLine();
	SmtpSession::SmtpError GetStandardError();

	LineReader::LineAvailableSignal::Slot<SmtpProtocolCommand>	m_handleResponseSlot;
	std::string		m_serverMessage;
	bool			m_inResponse;
	bool			m_firstResponse;
	bool			m_lastResponse;
	std::string		m_responseLine;
	int				m_responseLineNumber;
	int				m_statusCode;
	StatusCode		m_status;
};

#endif /* SMTPPROTOCOLCOMMAND_H_ */
