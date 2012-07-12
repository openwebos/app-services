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

#ifndef SEARCHFOLDERCOMMAND_H_
#define SEARCHFOLDERCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "ImapCoreDefs.h"
#include <vector>

class FetchResponseParser;
class UidSearchResponseParser;
class SearchRequest;

class SearchFolderCommand : public ImapSessionCommand
{
public:
	SearchFolderCommand(ImapSession& session, const MojObject& folderId, const MojRefCountedPtr<SearchRequest>& searchRequest);
	virtual ~SearchFolderCommand();

protected:
	void RunImpl();
	MojErr HandleContinuation();
	MojErr HandleSearchResponse();
	void RequestHeaders();
	MojErr HandleHeadersResponse();

	void Done();

	bool Cancel(CancelType cancelReason);

	void Failure(const std::exception& e);

	MojObject								m_folderId;
	MojRefCountedPtr<SearchRequest>			m_searchRequest;
	std::vector<UID>						m_matchingUIDs;

	MojRefCountedPtr<UidSearchResponseParser>	m_searchResponseParser;
	MojRefCountedPtr<FetchResponseParser>		m_headersResponseParser;

	MojSignal<>::Slot<SearchFolderCommand>	m_handleContinuationSlot;
	MojSignal<>::Slot<SearchFolderCommand>	m_searchResponseSlot;
	MojSignal<>::Slot<SearchFolderCommand>	m_headersResponseSlot;
};

#endif /* SEARCHFOLDERCOMMAND_H_ */
