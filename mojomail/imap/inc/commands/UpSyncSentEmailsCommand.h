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

#ifndef UPSYNCSENTEMAILSCOMMAND_H_
#define UPSYNCSENTEMAILSCOMMAND_H_

#include "commands/ImapSyncSessionCommand.h"
#include "commands/AppendCommand.h"


class StoreResponseParser;

class UpSyncSentEmailsCommand : public ImapSyncSessionCommand
{
	typedef boost::shared_ptr<Email> EmailPtr;

public:
	UpSyncSentEmailsCommand(ImapSession& session, const MojObject& folderId);
	virtual ~UpSyncSentEmailsCommand();

	void RunImpl();

protected:

	void GetSentFolderName();
	MojErr GetSentFolderNameResponse(MojObject& response, MojErr err);

	void GetSentEmails();
	MojErr GetSentEmailsResponse(MojObject& response, MojErr err);

	MojErr AppendCommandResponse();

	void PurgeSentEmail();
	MojErr PurgeSentEmailResponse(MojObject& response, MojErr err);

	void CreateAndRunAppendCommand();

	void UpSyncSentItemsComplete();

	MojErr ReplaceActivityDone();

	static const int LOCAL_BATCH_SIZE;

	MojRefCountedPtr<AppendCommand> m_appendCommand;
	MojRefCountedPtr<ImapCommandResult>	m_appendResult;

	MojRefCountedPtr<AppendResponseParser> 		m_appendToServerResponseParser;

	MojDbQuery::Page		m_sentEmailsPage;
	std::vector<EmailPtr> 	m_sentEmails;

	EmailPtr				m_emailP;

	// Local emails that need to be purged
	MojObject::ObjectVec	m_pendingPurge;

	std::string				m_sentFolderName;

	MojDbClient::Signal::Slot<UpSyncSentEmailsCommand>						m_getSentFolderNameSlot;
	MojDbClient::Signal::Slot<UpSyncSentEmailsCommand> 						m_getSentEmailsSlot;
	MojSignal<>::Slot<UpSyncSentEmailsCommand>								m_appendResponseSlot;
	MojDbClient::Signal::Slot<UpSyncSentEmailsCommand>						m_purgeSentEmailSlot;
	MojSignal<>::Slot<UpSyncSentEmailsCommand>								m_replaceActivitySlot;

};

#endif /* UPSYNCSENTEMAILSCOMMAND_H_ */
