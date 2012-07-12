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

#include "commands/DeleteActivitiesCommand.h"
#include "data/DatabaseAdapter.h"
#include "PopClient.h"
#include "PopDefs.h"
#include <boost/foreach.hpp>

DeleteActivitiesCommand::DeleteActivitiesCommand(PopClient& client)
: PopClientCommand(client, "Delete Activities"),
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
	MojLogTrace(m_log);

	MojObject payload(MojObject::TypeObject);

	m_client.SendRequest(m_activityListSlot, "com.palm.activitymanager", "list", payload);
}

MojErr DeleteActivitiesCommand::GetActivityListResponse(MojObject& response, MojErr err)
{
	MojLogTrace(m_log);

	try {
		MojObject activities;
		err = response.getRequired("activities", activities);
		ErrorToException(err);

		MojString creatorIdSubstring;
		err = creatorIdSubstring.assign("com.palm.pop");
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

			fprintf(stderr, "activity id=%s name=%s, creator=%s\n",
					AsJsonString(activityId).c_str(), name.data(), creator.data());

			// Check that the accountId and creator (com.palm.pop) are in the activity fields
			if(creator.find(creatorIdSubstring.data(), 0) != MojInvalidIndex
			&& ShouldDeleteActivity(activity)) {
				m_deleteIds.push(activityId);
			}
		}

		DeleteActivities();
	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in getting activity list response: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in activity list response");
		Failure(MailException("Unknown exception in activity list response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void DeleteActivitiesCommand::DeleteActivities()
{
	MojLogTrace(m_log);

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
	MojLogTrace(m_log);

	if(err) {
		// Ignore errors
		MojLogInfo(m_log, "couldn't cancel activity: %s",
				MojErrException(err, __FILE__, __LINE__).what());
	}

	try {
		// Delete the next activity, or move on
		DeleteActivities();
	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in delete activity response: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in delete activity response");
		Failure(MailException("Unknown exception in delete activity response", __FILE__, __LINE__));
	}

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
