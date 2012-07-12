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

#ifndef EXTENDEDHELLOCOMMAND_H_
#define EXTENDEDHELLOCOMMAND_H_

#include "commands/SmtpProtocolCommand.h"

class ExtendedHelloCommand : public SmtpProtocolCommand
{
public:
	static const char* const	COMMAND_STRING;
	static const char* const	SIZE_EXTENSION_KEYWORD;
	static const char* const	TLS_EXTENSION_KEYWORD;
	static const char* const	PIPELINING_EXTENSION_KEYWORD; 
	static const char* const	CHUNKING_EXTENSION_KEYWORD; 
	static const char* const	AUTH_EXTENSION_KEYWORD;
	static const char* const	PLAIN_AUTH_KEYWORD;
	static const char* const	LOGIN_AUTH_KEYWORD;
	static const char* const	YAHOO_AUTH_KEYWORD;

	ExtendedHelloCommand(SmtpSession& session, const std::string & serverName);
	virtual ~ExtendedHelloCommand();

	virtual void RunImpl();

private:
	virtual MojErr HandleResponse(const std::string&);
	virtual MojErr HandleMultilineResponse(const std::string& line, int lineNumber, bool lastLine);
	
	std::string m_serverName;
	bool m_sawSizeExtension;
	int  m_sawSizeMax;
	bool m_sawTLSExtension;
	bool m_sawPipeliningExtension;
	bool m_sawChunkingExtension;
	bool m_sawAuthExtension;
	bool m_sawAuthPlain;
	bool m_sawAuthLogin;
	bool m_sawAuthYahoo;
	                      
};

#endif /* EXTENDEDHELLOCOMMAND_H_ */
