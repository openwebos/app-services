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

#ifndef SAVEEMAILCOMMAND_H_
#define SAVEEMAILCOMMAND_H_

#include "SmtpCommon.h"
#include "db/MojDbClient.h"
#include "SmtpClient.h"
#include "commands/SmtpCommand.h"
#include "core/MojServiceMessage.h"
#include "client/FileCacheClient.h"

/**
 * Adds an email to the outbox/drafts folder, and writes "content" fields to file cache.
 */
class SaveEmailCommand : public SmtpCommand
{
public:
	SaveEmailCommand(SmtpClient& client,
			MojRefCountedPtr<MojServiceMessage> message,
					 MojObject email,
					 MojObject accountId,
					 bool isDraft);
	virtual ~SaveEmailCommand();
	
	void RunImpl();
	void Cancel();
protected:
	static const int EMAIL_FILECACHE_COST;
	static const int EMAIL_NO_LIFE_TIME;
	static const int MAX_SUMMARY_LENGTH;
	
	// Find Outbox/Drafts folder
	void GetAccount();
	MojErr GetAccountResponse(MojObject &response, MojErr err);
	
	void PrepareParts();
	void CreateNextCacheObject();
	
	MojErr	FileCacheObjectInserted(MojObject &response, MojErr err);
	void	WriteToFileCache(MojObject &part, const MojString &pathName);
	
	void	PersistToDatabase();
	
	MojErr	PutDone(MojObject &response, MojErr err);
	
	// Report error back to the email app
	void	Error(const std::exception& e);
	
	SmtpClient&			m_client;
	MojRefCountedPtr<MojServiceMessage> m_message;
	
	MojObject			m_email;
	MojObject			m_accountId;
	MojObject			m_folderId;
	bool				m_isDraft;
	
	MojObject					m_partsArray;	// copy of parts array
	MojObject::ArrayIterator	m_partsIt;		// current part
	
	// Slots
	MojDbClient::Signal::Slot<SaveEmailCommand>		m_dbFolderSlot;
	MojDbClient::Signal::Slot<SaveEmailCommand>		m_dbPutSlot;
	FileCacheClient::ReplySignal::Slot<SaveEmailCommand>	m_fileCacheInsertedSlot;
};

#endif /*SAVEEMAILCOMMAND_H_*/
