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

#ifndef UPSYNCDRAFTSCOMMAND_H_
#define UPSYNCDRAFTSCOMMAND_H_

#include "commands/ImapSyncSessionCommand.h"
#include "db/MojDbClient.h"
#include "ImapCoreDefs.h"
#include <boost/shared_ptr.hpp>
#include <vector>

class ActivitySet;
class AppendResponseParser;
class AppendCommand;
class MoveEmailsCommand;
class ImapEmail;

class UpSyncDraftsCommand : public ImapSyncSessionCommand
{
	typedef boost::shared_ptr<ImapEmail> EmailPtr;

public:
	UpSyncDraftsCommand(ImapSession& session, const MojObject& folderId);
	virtual ~UpSyncDraftsCommand();

	virtual void RunImpl();

	virtual void Status(MojObject& status) const;

protected:

	void GetDraftsFolderName();
	MojErr GetDraftsFolderNameResponse(MojObject& response, MojErr err);

	void GetDrafts();
	MojErr GetDraftsResponse(MojObject& response, MojErr err);

	void CreateAndRunAppendCommand();
	MojErr AppendCommandResponse();

	void UpdateUidAndKind();
	MojErr UpdateUidAndKindResponse(MojObject& response, MojErr err);

	void DeleteOldDraftFromServer();
	MojErr DeleteOldDraftDone();

	void UpSyncDraftComplete();

	void SetupWatchDraftsActivity();
	MojErr ReplaceActivityDone();

	static const int LOCAL_BATCH_SIZE;

	MojRefCountedPtr<AppendCommand> m_appendCommand;
	MojRefCountedPtr<MoveEmailsCommand> m_moveCommand;

	MojRefCountedPtr<AppendResponseParser> 		m_appendToServerResponseParser;

	MojDbQuery::Page		m_draftsPage;
	std::vector<EmailPtr> 	m_drafts;

	EmailPtr				m_emailP;
	UID						m_oldUID;

	std::string				m_draftsFolderName;

	MojDbClient::Signal::Slot<UpSyncDraftsCommand>			m_getDraftsFolderNameSlot;
	MojDbClient::Signal::Slot<UpSyncDraftsCommand> 			m_getDraftsSlot;
	MojSignal<>::Slot<UpSyncDraftsCommand>					m_appendResponseSlot;
	MojDbClient::Signal::Slot<UpSyncDraftsCommand>			m_updateUidAndKindSlot;
	MojSignal<>::Slot<UpSyncDraftsCommand>					m_deleteOldDraftSlot;
	MojSignal<>::Slot<UpSyncDraftsCommand>					m_replaceActivitySlot;

};

#endif /* UPSYNCDRAFTSCOMMAND_H_ */
