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

#ifndef AUTODOWNLOADCOMMAND_H_
#define AUTODOWNLOADCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "db/MojDbClient.h"

/**
 * Command that searches for local emails with the autoDownload flag set
 * and queues fetch requests for them.
 */
class AutoDownloadCommand : public ImapSessionCommand
{
public:
	AutoDownloadCommand(ImapSession& session, const MojObject& folderId);
	virtual ~AutoDownloadCommand();

	void RunImpl();

	void GetAutoDownloads();
	MojErr GetAutoDownloadsResponse(MojObject& response, MojErr err);

protected:
	static const size_t MAX_AUTODOWNLOAD_SIZE;

	MojObject	m_folderId;
	MojDbQuery::Page	m_page;

	int			m_emailsExamined;

	MojDbClient::Signal::Slot<AutoDownloadCommand>	m_getAutoDownloadsSlot;
};

#endif /* AUTODOWNLOADCOMMAND_H_ */
