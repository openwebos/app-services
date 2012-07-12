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

#ifndef UNTAGGEDUPDATEHANDLER_H_
#define UNTAGGEDUPDATEHANDLER_H_

#include "protocol/ImapResponseParser.h"
#include "protocol/FetchResponseParser.h"

class HandleUpdateCommand;
class ImapCommandResult;

class UntaggedUpdateParser : public ImapResponseParser
{
public:
	UntaggedUpdateParser(ImapSession& session);
	virtual ~UntaggedUpdateParser();

	bool HandleUntaggedResponse(const std::string& line);

protected:
	MojErr UpdateCommandResponse();

	MojRefCountedPtr<HandleUpdateCommand>	m_handleUpdateCommand;
	MojRefCountedPtr<ImapCommandResult>		m_result;

	MojRefCountedPtr<FetchResponseParser>	m_fetchParser;
	MojSignal<>::Slot<UntaggedUpdateParser>	m_updateCommandSlot;
};

#endif /* UNTAGGEDUPDATEHANDLER_H_ */
