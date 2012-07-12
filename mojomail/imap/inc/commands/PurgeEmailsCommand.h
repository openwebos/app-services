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

#ifndef PURGEEMAILSCOMMAND_H_
#define PURGEEMAILSCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "core/MojObject.h"
#include "db/MojDbClient.h"

class ImapClient;

/**
 * Purges all emails in a folder
 */
class PurgeEmailsCommand : public ImapClientCommand
{
public:
	PurgeEmailsCommand(ImapClient& client, const MojObject& folderId);
	virtual ~PurgeEmailsCommand();

	void RunImpl();
	std::string Describe() const;

protected:

	MojErr PurgeResponse(MojObject& response, MojErr err);

	MojObject	m_folderId;

	MojDbClient::Signal::Slot<PurgeEmailsCommand>	m_purgeSlot;
};

#endif /* PURGEEMAILSCOMMAND_H_ */
