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

#include "commands/CheckOutboxCommand.h"
#include "ImapClient.h"
#include "data/DatabaseInterface.h"
#include "ImapPrivate.h"
#include "data/DatabaseAdapter.h"
#include "activity/ActivityBuilder.h"
#include "activity/ActivitySet.h"
#include "activity/ImapActivityFactory.h"

CheckOutboxCommand::CheckOutboxCommand(ImapClient& client, SyncParams syncParams)
: ImapClientCommand(client),
  m_syncParams(syncParams),
  m_activitySet(new ActivitySet(m_client)),
  m_getSentEmailsSlot(this, &CheckOutboxCommand::GetSentEmailsResponse),
  m_getToDeleteSlot(this, &CheckOutboxCommand::GetEmailsToDeleteResponse),
  m_purgeSlot(this, &CheckOutboxCommand::PurgeResponse),
  m_endActivitiesSlot(this, &CheckOutboxCommand::EndActivitiesDone)
{
	m_activitySet->AddActivities(syncParams.GetSyncActivities());
}

CheckOutboxCommand::~CheckOutboxCommand()
{
}

void CheckOutboxCommand::RunImpl()
{
	CommandTraceFunction();

	bool uploadEmails = true;

	if(!IsValidId(m_client.GetAccount().GetSentFolderId())) {
		// No sent folder
		uploadEmails = false;
	} else if(m_client.GetAccount().IsGoogle()) {
		// FIXME: should check if SMTP username and server is the same
		uploadEmails = false;
	}

	if(uploadEmails)
		GetSentEmails();
	else
		GetEmailsToDelete();
}

void CheckOutboxCommand::GetSentEmails()
{
	MojObject outboxId = m_client.GetAccount().GetOutboxFolderId();
	m_client.GetDatabaseInterface().GetSentEmails(m_getSentEmailsSlot, outboxId, m_sentEmailsPage, 1);
}

MojErr CheckOutboxCommand::GetSentEmailsResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		if(results.size() > 0) {
			// There's at least one sent email in the outbox
			// Ask client to fire up a session to upload the sent emails
			m_client.UploadSentEmails(m_syncParams.GetActivity());
			Complete();
		} else {
			EndActivities();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void CheckOutboxCommand::GetEmailsToDelete()
{
	CommandTraceFunction();

	MojObject outboxId = m_client.GetAccount().GetOutboxFolderId();
	m_client.GetDatabaseInterface().GetSentEmails(m_getToDeleteSlot, outboxId, m_sentEmailsPage);
}

MojErr CheckOutboxCommand::GetEmailsToDeleteResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		MojObject::ObjectVec ids;

		BOOST_FOREACH(const MojObject& obj, DatabaseAdapter::GetResultsIterators(response)) {
			MojObject id;
			err = obj.getRequired(DatabaseAdapter::ID, id);
			ErrorToException(err);

			// FIXME double-check that the email has been sent

			ids.push(id);
		}

		m_client.GetDatabaseInterface().DeleteEmailIds(m_purgeSlot, ids);
	} CATCH_AS_FAILURE

	return MojErrNone;
}

MojErr CheckOutboxCommand::PurgeResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		EndActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void CheckOutboxCommand::EndActivities()
{
	CommandTraceFunction();

	m_activitySet->AddActivities(m_syncParams.GetSyncActivities());
	m_activitySet->AddActivities(m_syncParams.GetConnectionActivities());

	ImapActivityFactory factory;
	ActivityBuilder ab;

	factory.BuildOutboxWatch(ab, m_client.GetAccountId(), m_client.GetAccount().GetOutboxFolderId(), 0);

	// Update outbox watch activity
	m_activitySet->ReplaceActivity(ab.GetName(), ab.GetActivityObject());
	m_activitySet->EndActivities(m_endActivitiesSlot);
}

MojErr CheckOutboxCommand::EndActivitiesDone()
{
	Complete();

	return MojErrNone;
}
