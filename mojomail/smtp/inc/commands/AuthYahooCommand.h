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

#ifndef AUTHYAHOOCOMMAND_H_
#define AUTHYAHOOCOMMAND_H_

#include <string>
#include "commands/SmtpProtocolCommand.h"

class AuthYahooCommand : public SmtpProtocolCommand
{
public:
	static const char* const	COMMAND_STRING;

	AuthYahooCommand(SmtpSession& session);
	virtual ~AuthYahooCommand();

	virtual void RunImpl();
private:
	virtual MojErr HandleResponse(const std::string& line);
	MojErr GetYahooCookiesSlot(MojObject& response, MojErr err);
	unsigned long long timeMillis();
	
	void WriteCommand();
	MojServiceRequest::ReplySignal::Slot<AuthYahooCommand> m_getYahooCookiesSlot;
	
	enum {	
		State_SendingYCookie,
		State_SendingTCookie
	}	m_state;
	
	MojString	m_tCookie;
	MojString	m_yCookie;
	MojString	m_partnerId;
	MojString	m_deviceId;
};

#endif /* AUTHYAHOOCOMMAND_H_ */
