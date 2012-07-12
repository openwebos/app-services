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

#include <iostream>
#include "commands/ExtendedHelloCommand.h"

const char* const ExtendedHelloCommand::COMMAND_STRING				= "EHLO";
const char* const ExtendedHelloCommand::SIZE_EXTENSION_KEYWORD		= "SIZE";
const char* const ExtendedHelloCommand::TLS_EXTENSION_KEYWORD		= "STARTTLS";
const char* const ExtendedHelloCommand::CHUNKING_EXTENSION_KEYWORD	= "CHUNKING";
const char* const ExtendedHelloCommand::PIPELINING_EXTENSION_KEYWORD= "PIPELINING";
const char* const ExtendedHelloCommand::AUTH_EXTENSION_KEYWORD		= "AUTH";
const char* const ExtendedHelloCommand::PLAIN_AUTH_KEYWORD			= "PLAIN";
const char* const ExtendedHelloCommand::LOGIN_AUTH_KEYWORD			= "LOGIN";
const char* const ExtendedHelloCommand::YAHOO_AUTH_KEYWORD			= "XYMCOOKIE";

ExtendedHelloCommand::ExtendedHelloCommand(SmtpSession& session, const std::string & serverName)
: SmtpProtocolCommand(session),
  m_serverName(serverName),
  m_sawSizeExtension(false),
  m_sawSizeMax(0),
  m_sawTLSExtension(false),
  m_sawPipeliningExtension(false),
  m_sawChunkingExtension(false),
  m_sawAuthExtension(false),
  m_sawAuthPlain(false),
  m_sawAuthLogin(false),
  m_sawAuthYahoo(false)
{
}

ExtendedHelloCommand::~ExtendedHelloCommand()
{

}

void ExtendedHelloCommand::RunImpl()
{
	SendCommand(std::string(COMMAND_STRING) + " " + m_serverName);
	
	m_sawSizeExtension = false;
	m_sawSizeMax = 0;
	m_sawTLSExtension = false;
	m_sawChunkingExtension = false;
	m_sawPipeliningExtension = false;
	m_sawAuthExtension = false;
	m_sawAuthPlain = false; 
	m_sawAuthLogin = false; 
	m_sawAuthYahoo = false;
}

MojErr ExtendedHelloCommand::HandleMultilineResponse(const std::string& line, int lineNumber, bool lastLine)
{
	// lineNumber == 0 is greeting
	
	if (lineNumber > 0) {
		size_t pos = line.find(" ");
		string keyword, rest;
		if (pos != string::npos) {
			keyword = line.substr(0,pos);
			rest = line.substr(pos+1);
		} else {
			keyword = line;
			rest = "";
		}
		
		// Assumes C_LOCALE
		if (strcasecmp(keyword.c_str(), SIZE_EXTENSION_KEYWORD) == 0) {
			m_sawSizeExtension = true;
			m_sawSizeMax = atoi(rest.c_str());
		} else if (strcasecmp(keyword.c_str(), TLS_EXTENSION_KEYWORD) == 0) {
			m_sawTLSExtension = true;
		} else if (strcasecmp(keyword.c_str(), CHUNKING_EXTENSION_KEYWORD) == 0) {
			m_sawChunkingExtension = true;
		} else if (strcasecmp(keyword.c_str(), PIPELINING_EXTENSION_KEYWORD) == 0) {
			m_sawPipeliningExtension = true;
		} else if (strcasecmp(keyword.c_str(), AUTH_EXTENSION_KEYWORD) == 0) {
			m_sawAuthExtension = true;
			
			while (rest.length() > 0) {
				string authkeyword;
				size_t s_pos = rest.find(" ");
				
				if (s_pos != string::npos) {
					authkeyword = rest.substr(0,s_pos);
					rest = rest.substr(s_pos+1);
				} else {
					authkeyword = rest;
					rest = "";
				}
				
				if (strcasecmp(authkeyword.c_str(), PLAIN_AUTH_KEYWORD) == 0) {
					m_sawAuthPlain = true;
				} else if (strcasecmp(authkeyword.c_str(), LOGIN_AUTH_KEYWORD) == 0) {
					m_sawAuthLogin = true;
				} else if (strcasecmp(authkeyword.c_str(), YAHOO_AUTH_KEYWORD) == 0) {
					m_sawAuthYahoo = true;
				} 
			}
		} 
	}
	
	return MojErrNone;
}

MojErr ExtendedHelloCommand::HandleResponse(const std::string& line)
{
	if (m_statusCode == 250) {
		m_session.HasSizeExtension(m_sawSizeExtension);
		m_session.HasSizeMax(m_sawSizeMax);
		m_session.HasTLSExtension(m_sawTLSExtension);
		m_session.HasChunkingExtension(m_sawChunkingExtension);
		m_session.HasPipeliningExtension(m_sawPipeliningExtension);
		m_session.HasAuthExtension(m_sawAuthExtension);
		m_session.HasPlainAuth(m_sawAuthPlain);
		m_session.HasLoginAuth(m_sawAuthLogin);
		m_session.HasYahooAuth(m_sawAuthYahoo);

		m_status = Status_Ok;
		
		m_session.HelloSuccess();
	} else { 
		m_status = Status_Err;

		SmtpSession::SmtpError error = GetStandardError();
		error.internalError = "EHLO failure";
		m_session.HelloFailure(error);
	}
		
	Complete();

	return MojErrNone;
}
