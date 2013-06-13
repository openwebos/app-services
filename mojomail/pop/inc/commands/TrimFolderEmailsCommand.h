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

#ifndef TRIMFOLDEREMAILSCOMMAND_H_
#define TRIMFOLDEREMAILSCOMMAND_H_

#include "commands/PopSessionCommand.h"
#include "core/MojObject.h"
#include "data/DatabaseInterface.h"
#include "data/UidCache.h"

class TrimFolderEmailsCommand : public PopSessionCommand
{
public:
	static const int LOAD_EMAIL_BATCH_SIZE;  // batch size to load emails from database

	TrimFolderEmailsCommand(PopSession& session, const MojObject& folderId, int trimCount, UidCache& uidCache);
	~TrimFolderEmailsCommand();

private:
	virtual void 	RunImpl();
	void 			GetLocalEmails();
	MojErr			GetLocalEmailsResponse(MojObject& response, MojErr err);
	void			DeleteLocalEmails();
	MojErr			DeleteLocalEmailsResponse(MojObject& response, MojErr err);


	MojObject 											m_folderId;
	int													m_numberToTrim;
	MojDbQuery::Page									m_localEmailsPage;
	MojObject::ObjectVec								m_idsToTrim;
	UidCache& 											m_uidCache;
	MojDbClient::Signal::Slot<TrimFolderEmailsCommand> 	m_getLocalEmailsResponseSlot;
	MojDbClient::Signal::Slot<TrimFolderEmailsCommand> 	m_deleteLocalEmailsResponseSlot;
};

#endif /* TRIMFOLDEREMAILSCOMMAND_H_ */
