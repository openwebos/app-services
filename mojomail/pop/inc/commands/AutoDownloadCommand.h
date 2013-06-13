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

#ifndef AUTODOWNLOADCOMMAND_H_
#define AUTODOWNLOADCOMMAND_H_

#include "client/DownloadListener.h"
#include "commands/PopSessionPowerCommand.h"
#include "core/MojObject.h"
#include "db/MojDbClient.h"

class AutoDownloadCommand : public PopSessionPowerCommand
{
public:
	const static int MAX_EMAIL_BODIES = 100;	// download bodies from N most recent emails; -1 means all, 0 means none
	const static int BATCH_SIZE = 100;			// how many emails to request from the db at at time (must be <= 500)

	AutoDownloadCommand(PopSession& session, const MojObject& folderId);
	virtual ~AutoDownloadCommand();

	virtual void RunImpl();
	MojErr	GetEmailsToFetchResponse(MojObject& response, MojErr err);

private:
	void		 GetEmailsToFetch();
	virtual void Complete();
	virtual	void Failure(const std::exception& ex);

	MojObject										m_folderId;
	MojInt64										m_lastSyncRev;
	int												m_numEmailsExamined;
	int												m_maxBodies;
	int												m_numDownloadRequests;
	MojDbQuery::Page								m_emailsPage;
	boost::shared_ptr<DownloadListener>				m_listener;
	MojDbClient::Signal::Slot<AutoDownloadCommand> 	m_getEmailsToFetchResponseSlot;
};

#endif /* AUTODOWNLOADCOMMAND_H_ */
