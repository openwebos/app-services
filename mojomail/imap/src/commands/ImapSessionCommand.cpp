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

#include "commands/ImapSessionCommand.h"
#include "client/ImapSession.h"
#include "activity/ActivitySet.h"
#include "ImapPrivate.h"

using namespace std;

ImapSessionCommand::ImapSessionCommand(ImapSession& session, Priority priority)
: ImapCommand(session, priority, session.GetLogger()),
  m_session(session),
  m_commandActivitySet(new ActivitySet(session.GetBusClient())),
  m_readyToRun(false),
  m_startActivitiesSlot(this, &ImapSessionCommand::CommandActivityStarted)
{
}

ImapSessionCommand::~ImapSessionCommand()
{
}

// FIXME this could be more efficient
string ImapSessionCommand::QuoteString(const std::string& text)
{
	string out;

	out.push_back('"');
	for(std::string::const_iterator it = text.begin(); it != text.end(); ++it) {
		char c = *it;

		if(c == '"' || c == '\\')
			out.push_back('\\');

		out.push_back(c);
	}
	out.push_back('"');

	return out;
}

void ImapSessionCommand::AddActivity(const ActivityPtr& activity)
{
	if(activity.get()) {
		m_commandActivitySet->AddActivity(activity);

		// Attempt to adopt immediately (async) to avoid blocking ActivityManager queue
		activity->Adopt(m_session.GetBusClient());
	}
}

bool ImapSessionCommand::PrepareToRun()
{
	CommandTraceFunction();

	if(!ImapCommand::PrepareToRun()) {
		return false;
	}

	if(m_readyToRun) {
		return true;
	}

	if(!m_commandActivitySet->GetActivities().empty()) {
		// Adopt activity passed in from activity manager
		m_commandActivitySet->StartActivities(m_startActivitiesSlot);
		return false;
	} else {
		// Run without an activity
		m_readyToRun = true;
		return true;
	}
}

MojErr ImapSessionCommand::CommandActivityStarted()
{
	// We can run once the activity has started
	m_readyToRun = true;
	RunIfReady();

	return MojErrNone;
}

void ImapSessionCommand::Status(MojObject& status) const
{
	MojErr err;

	ImapCommand::Status(status);

	if(!m_commandActivitySet->GetActivities().empty()) {
		MojObject activitySetStatus;
		m_commandActivitySet->Status(activitySetStatus);

		err = status.put("commandActivitySet", activitySetStatus);
		ErrorToException(err);
	}
}

void ImapSessionCommand::Cleanup()
{
	m_commandActivitySet->ClearActivities();
	
	ImapCommand::Cleanup();
}

std::string ImapSessionCommand::Describe() const
{
	return ImapCommand::Describe();
}

std::string ImapSessionCommand::Describe(const MojObject& folderId) const
{
	return ImapCommand::Describe() + " [folderId=" + AsJsonString(folderId) + "]";
}
