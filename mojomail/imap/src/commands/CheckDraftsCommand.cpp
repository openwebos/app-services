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

#include "commands/CheckDraftsCommand.h"
#include "ImapClient.h"
#include "data/DatabaseInterface.h"
#include "ImapPrivate.h"

CheckDraftsCommand::CheckDraftsCommand(ImapClient& client, const ActivityPtr& activity)
: ImapClientCommand(client),
  m_activity(activity),
  m_getDraftsSlot(this, &CheckDraftsCommand::GetDraftsResponse)
{
}

CheckDraftsCommand::~CheckDraftsCommand()
{
}

void CheckDraftsCommand::RunImpl()
{
	CommandTraceFunction();

	GetDrafts();
}

void CheckDraftsCommand::GetDrafts()
{
	MojObject draftsFolderId = m_client.GetAccount().GetDraftsFolderId();
	m_client.GetDatabaseInterface().GetDrafts(m_getDraftsSlot, draftsFolderId, m_draftsPage, 1);
}

MojErr CheckDraftsCommand::GetDraftsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		if(results.size() > 0) {
			// There's at least one draft.
			// Ask client to fire up a session to upload it.
			m_client.UploadDrafts(m_activity);
			Complete();
		} else {
			// TODO: If there's no drafts folder, or we're using Gmail, we can just delete the emails right now
			Complete();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}
