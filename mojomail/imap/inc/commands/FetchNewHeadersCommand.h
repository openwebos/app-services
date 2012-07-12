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

#ifndef FETCHNEWHEADERSCOMMAND_H_
#define FETCHNEWHEADERSCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "protocol/FetchResponseParser.h"
#include "core/MojObject.h"
#include "db/MojDbClient.h"

class FetchResponseParser;

class FetchNewHeadersCommand : public ImapSessionCommand
{
public:
	FetchNewHeadersCommand(ImapSession& session, const MojObject& folderId);
	virtual ~FetchNewHeadersCommand();

	void RunImpl();

	static const std::string FETCH_ITEMS;

protected:
	void FetchNewHeaders();
	MojErr FetchResponse();

	MojErr PutEmailsResponse(MojObject& response, MojErr err);

	MojObject	m_folderId;
	std::vector<unsigned int>	m_msgNums;

	MojRefCountedPtr<FetchResponseParser>	m_fetchResponseParser;

	ImapResponseParser::DoneSignal::Slot<FetchNewHeadersCommand>	m_fetchSlot;
	MojDbClient::Signal::Slot<FetchNewHeadersCommand>				m_putEmailsSlot;
};

#endif /* FETCHNEWHEADERSCOMMAND_H_ */
