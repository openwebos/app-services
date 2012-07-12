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

#include "commands/DeleteActivitiesCommand.h"
#include "data/DatabaseAdapter.h"
#include "ImapPrivate.h"
#include "ImapClient.h"

DeleteActivitiesCommand::DeleteActivitiesCommand(ImapClient& client)
: ImapClientCommand(client),
  m_activityListSlot(this, &DeleteActivitiesCommand::GetActivityListResponse),
  m_deleteActivitySlot(this, &DeleteActivitiesCommand::DeleteActivityResponse)
{
}

DeleteActivitiesCommand::~DeleteActivitiesCommand()
{
}

void DeleteActivitiesCommand::RunImpl()
{
	GetActivityList();
}

void DeleteActivitiesCommand::GetActivityList()
{
	CommandTraceFunction();

	MojObject payload(MojObject::TypeObject);

	m_client.SendRequest(m_activityListSlot, "com.palm.activitymanager", "list", payload);
}

MojErr DeleteActivitiesCommand::GetActivityListResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		MojObject activities;
		err = response.getRequired("activities", activities);
		ErrorToException(err);

		MojString creatorIdSubstring;
		err = creatorIdSubstring.assign("com.palm.imap");
		ErrorToException(err);

		BOOST_FOREACH(const MojObject& activity, DatabaseAdapter::GetArrayIterators(activities)) {
			MojObject activityId;
			err = activity.getRequired("activityId", activityId);
			ErrorToException(err);

			MojString name;
			err = activity.getRequired("name", name);
			ErrorToException(err);

			MojString creator;
			err = activity.getRequired("creator", creator);
			ErrorToException(err);

			// Check that the accountId and creator (com.palm.imap) are in the activity fields
			if(creator.find(creatorIdSubstring.data(), 0) != MojInvalidIndex
			&& ShouldDeleteActivity(activity)) {
				fprintf(stderr, "preparing to delete activity id=%s name=%s, creator=%s\n",
					AsJsonString(activityId).c_str(), name.data(), creator.data());
				m_deleteIds.push(activityId);
			}
		}

		DeleteActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void DeleteActivitiesCommand::DeleteActivities()
{
	CommandTraceFunction();

	if(!m_deleteIds.empty()) {
		MojObject activityId = m_deleteIds.back();

		MojErr err = m_deleteIds.pop();
		ErrorToException(err);

		MojLogInfo(m_log, "deleting activity id %s", AsJsonString(activityId).c_str());

		MojObject payload;

		err = payload.put("activityId", activityId);
		ErrorToException(err);

		// Cancel slot, since we may be called from within a callback
		m_deleteActivitySlot.cancel();
		m_client.SendRequest(m_deleteActivitySlot, "com.palm.activitymanager", "cancel", payload);
	} else {
		Complete();
	}
}

MojErr DeleteActivitiesCommand::DeleteActivityResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);
	} catch(const std::exception& e) {
		// Ignore errors
		MojLogInfo(m_log, "couldn't cancel activity: %s", e.what());
	}

	try {
		// Delete the next activity, or move on
		DeleteActivities();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

bool DeleteActivitiesCommand::ShouldDeleteActivity(const MojObject& activity)
{
	MojErr err;
	MojString name;

	err = activity.getRequired("name", name);
	ErrorToException(err);

	if(!m_includeFilter.empty()) {
		bool nameMatches = name.find(m_includeFilter) != MojInvalidIndex;
		if(nameMatches && !m_excludeFilter.empty()) {
			nameMatches = name.find(m_excludeFilter) == MojInvalidIndex;
		}
		return nameMatches;
	}

	return false;
}
