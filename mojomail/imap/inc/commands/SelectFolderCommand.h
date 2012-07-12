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

#ifndef SELECTFOLDERCOMMAND_H_
#define SELECTFOLDERCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "protocol/ImapResponseParser.h"
#include "db/MojDbClient.h"

class ExamineResponseParser;
class PurgeEmailsCommand;

class SelectFolderCommand : public ImapSessionCommand
{
public:
	SelectFolderCommand(ImapSession& session, const MojObject& folderId);
	virtual ~SelectFolderCommand();
	
	void RunImpl();
	std::string Describe() const;

protected:
	void GetFolder();
	MojErr GetFolderResponse(MojObject& response, MojErr err);

	void SelectFolder();
	MojErr SelectResponse();
	
	void PurgeEmails();
	MojErr PurgeEmailsDone();

	void UpdateFolder();
	MojErr UpdateFolderResponse(MojObject& response, MojErr err);

	void SelectDone();

	void Failure(const std::exception& e);

	ImapFolderPtr	m_folder;
	MojObject 		m_selectFolderId;
	bool			m_initialSync;

	MojRefCountedPtr<ExamineResponseParser>	m_responseParser;
	MojRefCountedPtr<PurgeEmailsCommand>	m_purgeEmailsCommand;

	MojDbClient::Signal::Slot<SelectFolderCommand>				m_getFolderResponseSlot;
	ImapResponseParser::DoneSignal::Slot<SelectFolderCommand>	m_selectFolderResponseSlot;
	MojSignal<>::Slot<SelectFolderCommand>						m_purgeEmailsSlot;
	MojDbClient::Signal::Slot<SelectFolderCommand>				m_updateFolderSlot;
};

#endif /*SELECTFOLDERCOMMAND_H_*/
