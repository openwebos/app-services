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

#ifndef DELETEACTIVITIESCOMMAND_H_
#define DELETEACTIVITIESCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "core/MojObject.h"
#include "core/MojServiceRequest.h"

class DeleteActivitiesCommand : public ImapClientCommand
{
public:
	DeleteActivitiesCommand(ImapClient& client);
	virtual ~DeleteActivitiesCommand();

	// Set activity name substring to match for activities that *should* be deleted
	void SetIncludeNameFilter(const MojString& substring) { m_includeFilter = substring; }

	// Set activity name substring to match for activities that should *not* be deleted
	void SetExcludeNameFilter(const MojString& substring) { m_excludeFilter = substring; }

protected:
	void RunImpl();

	void GetActivityList();
	MojErr GetActivityListResponse(MojObject& response, MojErr err);

	virtual bool ShouldDeleteActivity(const MojObject& obj);

	void DeleteActivities();
	MojErr DeleteActivityResponse(MojObject& response, MojErr err);

	MojObject::ObjectVec m_deleteIds;
	MojString m_includeFilter;
	MojString m_excludeFilter;

	MojServiceRequest::ReplySignal::Slot<DeleteActivitiesCommand>	m_activityListSlot;
	MojServiceRequest::ReplySignal::Slot<DeleteActivitiesCommand>	m_deleteActivitySlot;
};

#endif /* DELETEACTIVITIESCOMMAND_H_ */
