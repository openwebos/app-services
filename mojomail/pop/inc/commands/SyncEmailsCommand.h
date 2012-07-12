// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef SYNCEMAILSCOMMAND_H_
#define SYNCEMAILSCOMMAND_H_

#include "commands/DeleteLocalEmailsCommand.h"
#include "commands/DeleteServerEmailsCommand.h"
#include "commands/DownloadEmailHeaderCommand.h"
#include "commands/DownloadEmailPartsCommand.h"
#include "commands/FetchEmailCommand.h"
#include "commands/HandleRequestCommand.h"
#include "commands/InsertEmailsCommand.h"
#include "commands/LoadUidCacheCommand.h"
#include "commands/PopSessionCommand.h"
#include "commands/ReconcileEmailsCommand.h"
#include "commands/TrimFolderEmailsCommand.h"
#include "commands/UpdateUidCacheCommand.h"
#include "db/MojDbClient.h"
#include "core/MojObject.h"
#include "data/UidCache.h"
#include "data/PopEmail.h"
#include "data/UidMap.h"
#include <map>

/**
 * A command object that is responsible for syncing a folder in a POP account.
 *
 * There are five folders created for each POP account: INBOX, DRAFT, OUTBOX,
 * SENT, and TRASH folders.  Only INBOX is syncable by this command.  DRAFT,
 * SENT and TRASH folders are local folders, so syncing these folders via this
 * command will yield a no-op result.  OUTBOX will be synced by SyncOutboxCommand.
 */
class SyncEmailsCommand : public HandleRequestCommand
{
public:
	typedef MojSignal1<bool>	DoneSignal;

	SyncEmailsCommand(PopSession& sesion, const MojObject& folderId, boost::shared_ptr<UidMap>& uidMap);
	virtual ~SyncEmailsCommand();

	virtual void RunImpl();

	static const int LOAD_EMAIL_BATCH_SIZE;  // batch size to load emails from database
	static const int SAVE_EMAIL_BATCH_SIZE;  // batch size to persist emails to database
	static const int SECONDS_IN_A_DAY;

	MojErr	ReconcileEmailsResponse();
	MojErr	DeleteLocalEmailsResponse();
	MojErr	GetEmailHeaderResponse(bool failed);
	MojErr	SaveEmailsResponse();
	MojErr	TrimEmailsResponse();
	MojErr	DeleteServerEmailsResponse();
	MojErr	LoadUidCacheResponse();
	MojErr	SaveUidCacheResponse();

private:
	typedef enum {
		State_None,
		State_ReconcileEmails,
		State_DeleteLocalEmails,
		State_GetNextMessageToDownloadHeader,
		State_DownloadEmailHeader,
		State_PersistEmails,
		State_TrimEmails,
		State_DeleteServerEmails,
		State_LoadLatestUidCache,
		State_SaveUidCache,
		State_HandleRequest,
		State_Complete,
		State_Cancel
	} CommandState;

	MojInt64		GetCutOffTime(int lookbackDays);
	MojErr			SyncSessionReadyResponse();
	void			ReconcileEmails();
	void			DeleteLocalEmails();
	void			GetNextMessageToDownloadHeader();
	void			FetchEmailHeader(const ReconcileEmailsCommand::ReconcileInfoPtr& info);
	bool			ShouldIncludeInFolder(const MojInt64& timestamp);
	MojErr			SaveEmails(PopEmail::PopEmailPtrVectorPtr emails);
	void 			GetDownloadBodyMessages();
	void			GetNextMessageToDownloadBody();
	void			CheckDownloadState(const MojInt64& timestamp);
	void			TrimEmails();
	void 			DeleteServerEmails();
	void			LoadLatestUidCache();
	void			SaveUidCache();
	virtual void 	HandleNextRequest();
	void			CheckState();
	void			CommandComplete();
	virtual	void	Failure(const std::exception& ex);

	virtual MojErr 	HandleRequestResponse();

	boost::shared_ptr<PopAccount>					m_account;
	MojObject 										m_folderId;
	CommandState									m_state;
	CommandState									m_orgState;			// the state that this command is in before it handles on-demand request
	int												m_syncWindow;
	MojInt64										m_cutOffTime;
	boost::shared_ptr<UidMap>						m_uidMap;
	UidCache										m_uidCache;
	UidCache										m_latestUidCache;
	ReconcileEmailsCommand::ReconcileInfoQueue		m_reconcileEmails;
	MojObject::ObjectVec							m_expiredEmailIds;  // IDs of emails that are just beyond sync window
	MojObject::ObjectVec							m_serverDeletedEmailIds;
	ReconcileEmailsCommand::LocalDeletedEmailsVec	m_localDeletedEmailUids;
	std::set<ReconcileEmailsCommand::LocalDeletedEmailInfo>	m_alreadyDeletedUids;
	ReconcileEmailsCommand::ReconcileInfoPtr		m_reconcileInfo;
	int												m_visitedEmailHeadersCount;  // the total number of email headers that has been visited before we persist emails to database.
	PopEmail::PopEmailPtrVectorPtr				 	m_persistEmails;
	MojInt64										m_latestEmailTimestamp;
	MojInt64										m_prevEmailTimestamp;
	bool											m_wasMsgsOrdered;
	int												m_lookbackCount;
	int												m_totalMessageCount;

	MojRefCountedPtr<PopCommandResult>				m_commandResult;

	MojRefCountedPtr<ReconcileEmailsCommand>		m_reconcileEmailsCommand;
	MojRefCountedPtr<DeleteLocalEmailsCommand>		m_deleteLocalEmailsCommand;
	MojRefCountedPtr<InsertEmailsCommand>		 	m_insertEmailsCommand;
	MojRefCountedPtr<DownloadEmailHeaderCommand> 	m_downloadHeaderCommand;
	MojRefCountedPtr<DownloadEmailPartsCommand> 	m_downloadBodyCommand;
	MojRefCountedPtr<DeleteServerEmailsCommand> 	m_delServerEmailCommand;
	MojRefCountedPtr<FetchEmailCommand>				m_fetchEmailCommand;
	MojRefCountedPtr<TrimFolderEmailsCommand>		m_trimEmailsCommand;
	MojRefCountedPtr<LoadUidCacheCommand>			m_loadUidCacheCommand;
	MojRefCountedPtr<UpdateUidCacheCommand>			m_saveUidCacheCommand;

	MojSignal<>::Slot<SyncEmailsCommand> 			m_readyResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand> 			m_reconcileResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand> 			m_deleteLocalEmailsResponseSlot;
	DoneSignal::Slot<SyncEmailsCommand> 			m_getEmailHeaderResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand>		 	m_saveEmailsResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand>		 	m_trimEmailsResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand> 			m_deleteServerEmailsResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand> 			m_loadUidCacheResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand> 			m_saveUidCacheResponseSlot;
	MojSignal<>::Slot<SyncEmailsCommand> 			m_handleRequestResponseSlot;
};

#endif /* SYNCEMAILSCOMMAND_H_ */
