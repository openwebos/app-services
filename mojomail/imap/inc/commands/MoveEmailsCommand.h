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

#ifndef MOVEEMAILSCOMMAND_H_
#define MOVEEMAILSCOMMAND_H_

#include "commands/ImapSyncSessionCommand.h"
#include "db/MojDbClient.h"
#include "ImapCoreDefs.h"
#include <vector>

class ImapResponseParser;

class MoveEmailsCommand : public ImapSyncSessionCommand
{
public:
	MoveEmailsCommand(ImapSession& session, const MojObject& srcFolderId, const MojObject& destFolderId, bool deleteEmails, const std::vector<UID>& uids, const MojObject::ObjectVec& ids);
	virtual ~MoveEmailsCommand();

protected:
	void RunImpl();

	void GetDestFolder();
	MojErr GetDestFolderResponse(MojObject& response, MojErr err);

	void CopyToDest(const MojString& destfolderName);
	MojErr CopyToDestResponse();

	void CopyFailed();

	void DeleteFromServer();
	MojErr DeleteFromServerResponse();

	void Expunge();
	MojErr ExpungeResponse();

	void PurgeEmails();
	MojErr PurgeEmailsResponse(MojObject& response, MojErr err);

	void UnmoveEmails();
	MojErr UnmoveEmailsResponse(MojObject& response, MojErr err);

	MojObject				m_destFolderId;
	bool					m_deleteEmails;
	std::vector<UID>		m_uids;
	MojObject::ObjectVec	m_ids;

	MojRefCountedPtr<ImapResponseParser>		m_copyResponseParser;
	MojRefCountedPtr<ImapResponseParser>		m_deleteResponseParser;
	MojRefCountedPtr<ImapResponseParser>		m_expungeResponseParser;

	MojDbClient::Signal::Slot<MoveEmailsCommand> 			m_getDestFolderSlot;
	MojSignal<>::Slot<MoveEmailsCommand>					m_copyToDestSlot;
	MojSignal<>::Slot<MoveEmailsCommand>					m_deleteFromServerSlot;
	MojSignal<>::Slot<MoveEmailsCommand>					m_expungeSlot;
	MojDbClient::Signal::Slot<MoveEmailsCommand>			m_purgeSlot;
	MojDbClient::Signal::Slot<MoveEmailsCommand>			m_unmoveEmailsSlot;
};

#endif /* MOVEEMAILSCOMMAND_H_ */
