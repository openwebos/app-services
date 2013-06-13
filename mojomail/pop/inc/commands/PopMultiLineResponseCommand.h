// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#ifndef POPMULTILINERESPONSECOMMAND_H_
#define POPMULTILINERESPONSECOMMAND_H_

#include "commands/PopProtocolCommand.h"

class PopMultiLineResponseCommand : public PopProtocolCommand
{
public:
	static const char* const	TERMINATOR_CHARS;

	PopMultiLineResponseCommand(PopSession& session);
	virtual ~PopMultiLineResponseCommand();

protected:
	virtual MojErr	ReceiveResponse();
	virtual void Complete();

	/**
	 * Test a line to see whether it is the end of the response.
	 */
	bool 			isEndOfResponse(std::string line);

	void		PauseResponse();
	void		ResumeResponse();

	bool 		m_handleEndOfResponse; // if true, allows HandleResponse to deal with end of response
	bool		m_responsePaused;
	bool		m_complete;

	std::string m_terminationLine;
};

#endif /* POPMULTILINERESPONSECOMMAND_H_ */
