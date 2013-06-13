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

#ifndef DOWNLOADEMAILHEADERCOMMAND_H_
#define DOWNLOADEMAILHEADERCOMMAND_H_

#include "commands/PopMultiLineResponseCommand.h"

class AsyncEmailParser;

class DownloadEmailHeaderCommand : public PopMultiLineResponseCommand
{
public:
	static const char* const	COMMAND_STRING;

	typedef MojSignal1<bool>	HeaderDoneSignal;

	DownloadEmailHeaderCommand(PopSession& session, int msgNum,
			const EmailPtr& email, HeaderDoneSignal::SlotRef doneSlot);
	virtual ~DownloadEmailHeaderCommand();

	void 			RunImpl();
	virtual MojErr 	HandleResponse(const std::string& line);

protected:
	void			ParseFailed();
	virtual void 	Complete();
	virtual void	Cleanup();
	virtual void 	Failure(const std::exception& exc);

	int 					m_msgNum;
	MojRefCountedPtr<AsyncEmailParser>	m_emailParser;
	bool					m_parseFailed;
	HeaderDoneSignal		m_doneSignal;
};

#endif /* DOWNLOADEMAILHEADERCOMMAND_H_ */
