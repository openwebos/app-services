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

#include "commands/PopSessionCommand.h"
#include "stream/LineReader.h"

#ifndef POPPROTOCOLCOMMAND_H_
#define POPPROTOCOLCOMMAND_H_

class PopProtocolCommand : public PopSessionCommand
{
public:
	const static char* const	STATUS_STRING_OK;
	const static char* const	STATUS_STRING_ERR;
	const static char* const	CRLF;

	enum StatusCode {
		Status_Ok,
		Status_Err
	};
	PopProtocolCommand(PopSession& session, Priority priority = NormalPriority);
	virtual ~PopProtocolCommand();

	virtual MojErr	HandleResponse(const std::string& line) = 0;
protected:
	virtual void 	RunImpl() = 0;
	void 			SendCommand(const std::string& request);
	virtual MojErr	ReceiveResponse();
	virtual void 	ParseResponseFirstLine();
	virtual void	AnalyzeCommandResponse(const std::string& serverMessage);
	virtual void	AnalyzeHotmailResponse(const std::string& serverMessage);
	virtual void	AnalyzeMailException(const MailException& ex);
	bool			IsHotmail();

	LineReader::LineAvailableSignal::Slot<PopProtocolCommand>	m_handleResponseSlot;
	std::string		m_requestStr;
	std::string		m_responseFirstLine;
	bool			m_includesCRLF;
	StatusCode		m_status;
	std::string		m_serverMessage;
	MailError::ErrorCode	m_errorCode;
};

#endif /* POPPROTOCOLCOMMAND_H_ */
