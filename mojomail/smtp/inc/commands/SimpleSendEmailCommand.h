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

#ifndef SIMPLESENDEMAILCOMMAND_H_
#define SIMPLESENDEMAILCOMMAND_H_

#include <string>
#include <vector>
#include "commands/SmtpProtocolCommand.h"

class SimpleSendEmailCommand : public SmtpProtocolCommand
{
public:
	static const char* const	FROM_COMMAND_STRING;
	static const char* const	TO_COMMAND_STRING;
	static const char* const	DATA_COMMAND_STRING;

	SimpleSendEmailCommand(SmtpSession& session, const std::string& fromAddress, const std::vector<std::string>& toAddress, const std::string& data);
	virtual ~SimpleSendEmailCommand();

	virtual void RunImpl();
private:
	virtual MojErr HandleResponse(const std::string& line);

	int					m_state;
	unsigned int		m_toIdx;
	const std::string	m_fromAddress;
	const std::vector<std::string>	m_toAddress;
	const std::string	m_data;
};

#endif /* SIMPLESENDMAILCOMMAND_H_ */
