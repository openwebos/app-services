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

#ifndef AUTHYAHOOCOMMAND_H_
#define AUTHYAHOOCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "protocol/CapabilityResponseParser.h"
#include "util/Timer.h"

class AuthYahooCommand : public ImapSessionCommand
{
public:

	static const char* const AUTH_COMMAND_STRING;
	static const char* const ID_COMMAND_STRING;

	AuthYahooCommand(ImapSession& session);
	virtual ~AuthYahooCommand();

	void RunImpl();
	unsigned long long timeSec();

	void GetNduid();

	void GetYahooCookies();
	void GetYahooCookiesTimeout();
	MojErr GetYahooCookiesResponse(MojObject& response, MojErr err);

	void SendIDRequest();
	MojErr HandleIDResponse();

	void SendAuthenticateRequest();
	MojErr HandleAuthenticateContinue();

	void SendCookiesCommand();
	MojErr HandleAuthenticateDone();

	void Failure(const std::exception& e);
	void Cleanup();

protected:

	MojString	m_tCookie;
	MojString	m_yCookie;
	MojString	m_crumb;
	MojString	m_partnerId;
	MojString	m_deviceId;

	int			m_loginFailureCount;

	Timer<AuthYahooCommand>		m_getCookiesTimer;

	static const int FETCH_COOKIES_TIMEOUT;

	MojServiceRequest::ReplySignal::Slot<AuthYahooCommand> m_getYahooCookiesSlot;
	MojRefCountedPtr<ImapResponseParser> 		m_authYahooResponseParser;
	ImapResponseParser::ContinuationSignal::Slot<AuthYahooCommand>	m_authYahooDoResponseSlot;
	ImapResponseParser::ContinuationSignal::Slot<AuthYahooCommand>	m_authYahooAuthenticateContinueSlot;
	ImapResponseParser::DoneSignal::Slot<AuthYahooCommand>			m_authYahooAuthenticateDoneSlot;

};

#endif /* AUTHYAHOOCOMMAND_H_ */
