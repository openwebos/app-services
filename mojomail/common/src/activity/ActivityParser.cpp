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

#include "activity/ActivityParser.h"
#include "CommonPrivate.h"
#include "activity/Activity.h"

ActivityParser::ActivityParser()
{
}

ActivityParser::~ActivityParser()
{
}

void ActivityParser::ParseCallbackPayload(const MojObject& payload)
{
	if(payload.contains("$activity")) {
		MojErr err;
		err = payload.getRequired("$activity", m_activityInfo);
		ErrorToException(err);

		err = m_activityInfo.getRequired("activityId", m_activityId);
		ErrorToException(err);

		m_activityName.clear();
		MojString name;
		bool hasName;
		err = m_activityInfo.get("name", name, hasName);
		ErrorToException(err);

		if(hasName) {
			m_activityName = name;
		}

		// If this came from the database, make sure it's not an error
		if(m_activityInfo.contains("trigger")) {
			MojObject trigger;
			err = m_activityInfo.getRequired("trigger", trigger);
			ErrorToException(err);

			bool returnValue = false;
			err = trigger.getRequired("returnValue", returnValue);
			ErrorToException(err);

			if(!returnValue) {
				// FIXME
				//MojString triggerJson;
				//trigger.toJson(triggerJson);
				//MojLogWarning(s_log, "trigger returned false: %s", triggerJson.data());
				throw MailException("trigger returned false", __FILE__, __LINE__);
			}
		}
	}
}

bool ActivityParser::HasActivity() const
{
	return !m_activityId.undefined();
}

const MojObject& ActivityParser::GetActivityId() const
{
	return m_activityId;
}

const MojString& ActivityParser::GetActivityName() const
{
	return m_activityName;
}

const MojObject& ActivityParser::GetActivityInfo() const
{
	return m_activityInfo;
}

MojRefCountedPtr<Activity> ActivityParser::GetActivity() const
{
	if(HasActivity()) {
		ActivityPtr activity = Activity::PrepareAdoptedActivity(m_activityId);
		activity->SetName(m_activityName);
		activity->SetInfo(m_activityInfo);
		return activity;
	} else {
		return NULL;
	}
}

MojRefCountedPtr<Activity> ActivityParser::GetActivityFromPayload(const MojObject& payload)
{
	ActivityParser parser;
	parser.ParseCallbackPayload(payload);
	return parser.GetActivity();
}
