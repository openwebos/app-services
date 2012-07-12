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

#ifndef HANDLEUPDATECOMMAND_H_
#define HANDLEUPDATECOMMAND_H_

#include "commands/ImapSyncSessionCommand.h"
#include "db/MojDbClient.h"
#include "ImapCoreDefs.h"

class HandleUpdateCommand : public ImapSyncSessionCommand
{
public:
	HandleUpdateCommand(ImapSession& session, const MojObject& folderId, UID uid, bool deleted, const MojObject& newFlags);
	virtual ~HandleUpdateCommand();

protected:
	void RunImpl();

	void GetEmail();
	MojErr GetEmailResponse(MojObject& response, MojErr err);

	void DeleteEmail();
	MojErr DeleteResponse(MojObject& response, MojErr err);

	void UpdateFlags();
	MojErr UpdateFlagsResponse(MojObject& response, MojErr err);

	void Done();

	UID			m_uid;
	bool		m_deleted;
	MojObject	m_newFlags;

	MojObject	m_emailId;

	MojDbClient::Signal::Slot<HandleUpdateCommand>	m_getEmailSlot;
	MojDbClient::Signal::Slot<HandleUpdateCommand>	m_deleteSlot;
	MojDbClient::Signal::Slot<HandleUpdateCommand>	m_updateFlagsSlot;
};

#endif /* HANDLEUPDATECOMMAND_H_ */
