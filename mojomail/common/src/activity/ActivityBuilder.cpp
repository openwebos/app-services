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

#include "activity/ActivityBuilder.h"
#include "CommonPrivate.h"

const char* const ActivityBuilder::PRIORITY_LOW		= "low";
const char* const ActivityBuilder::PRIORITY_NORMAL	= "normal";
const char* const ActivityBuilder::PRIORITY_HIGH	= "high";

ActivityBuilder::ActivityBuilder()
: m_persist(false),
  m_explicit(false),
  m_foreground(false),
  m_immediate(false),
  m_power(false),
  m_powerDebounce(false)
{
}

ActivityBuilder::~ActivityBuilder()
{
}

MojString ActivityBuilder::GetName()
{
	MojString name;
	MojErr err = m_activityObject.getRequired("name", name);
	ErrorToException(err);
	return name;
}

void ActivityBuilder::SetName(const char* name)
{
	MojErr err = m_activityObject.putString("name", name);
	ErrorToException(err);
}

void ActivityBuilder::SetDescription(const char* desc)
{
	MojErr err = m_activityObject.putString("description", desc);
	ErrorToException(err);
}

void ActivityBuilder::SetPersist(bool persist)
{
	m_persist = persist;
}

void ActivityBuilder::SetExplicit(bool explicitStop)
{
	m_explicit = explicitStop;
}

void ActivityBuilder::SetCallback(const char* uri, const MojObject& params)
{
	MojErr err;
	MojObject callback;
	
	err = callback.putString("method", uri);
	ErrorToException(err);
	err = callback.put("params", params);
	ErrorToException(err);
	
	err = m_activityObject.put("callback", callback);
	ErrorToException(err);
}

void ActivityBuilder::SetDatabaseWatchTrigger(const MojDbQuery& query)
{
	MojErr err;
	
	MojObject queryObject;
	query.toObject(queryObject);
	
	MojObject triggerParams;
	err = triggerParams.put("query", queryObject);
	ErrorToException(err);
	
	MojObject trigger;
	err = trigger.putString("key", "fired");
	ErrorToException(err);
	err = trigger.putString("method", "palm://com.palm.db/watch");
	ErrorToException(err);
	err = trigger.put("params", triggerParams);
	ErrorToException(err);
	
	err = m_activityObject.put("trigger", trigger);
	ErrorToException(err);
}

void ActivityBuilder::DisableDatabaseWatchTrigger()
{
	MojErr err = m_activityObject.put("trigger", false);
	ErrorToException(err);
}

// Try to find an exact interval. Returns a blank string otherwise.
//
// "For a smart interval, the interval must be an even multiple of days,
// 12h, 6h, 3h, 1h, 30m, 15m, 10m, or 5m."
std::string ActivityBuilder::GetSmartInterval(int intervalSeconds)
{
	switch(intervalSeconds) {
	case 5 * 60: return "5m";
	case 10 * 60: return "10m";
	case 15 * 60: return "15m";
	case 30 * 60: return "30m";
	case 60 * 60: return "1h";
	case 3 * 60 * 60: return "3h";
	case 6 * 60 * 60: return "6h";
	case 12 * 60 * 60: return "12h";
	case 24 * 60 * 60: return "1d";
	default: return "";
	}
}

void ActivityBuilder::SetSyncInterval(int startOffsetSeconds, int intervalSeconds)
{
	MojErr err;
	char buf[255];
	MojObject schedule;

	if(startOffsetSeconds > 0) {
		time_t futureTime = time(NULL) + startOffsetSeconds;

		tm* gmtm = gmtime(&futureTime);

		strftime(buf, 255, "%Y-%m-%d %H:%M:%SZ", gmtm);
		MojString start;
		start.assign(buf);

		err = schedule.put("start", start);
		ErrorToException(err);
	}

	if(intervalSeconds > 0)
	{
		std::string smartInterval = GetSmartInterval(intervalSeconds);

		// Smart interval can only be used with no start time
		if(!smartInterval.empty() && startOffsetSeconds == 0) {
			err = schedule.putString("interval", smartInterval.c_str());
			ErrorToException(err);
		} else {
			// Get precise interval
			// FIXME
			throw MailException("precise intervals not implemented", __FILE__, __LINE__);
		}
	}

	err = m_activityObject.put("schedule", schedule);
	ErrorToException(err);
}

void ActivityBuilder::SetStartDelay(int startOffsetSeconds)
{
	MojErr err;
	MojObject schedule;

	// Get existing schedule object, if any
	m_activityObject.get("schedule", schedule);

	char buf[255];

	time_t futureTime = time(NULL) + startOffsetSeconds;

	tm* gmtm = gmtime(&futureTime);

	strftime(buf, 255, "%Y-%m-%d %H:%M:%SZ", gmtm);
	MojString start;
	start.assign(buf);

	err = schedule.put("start", start);
	ErrorToException(err);

	err = m_activityObject.put("schedule", schedule);
	ErrorToException(err);
}

void ActivityBuilder::SetSmartInterval(int intervalSeconds)
{
	MojErr err;
	MojObject schedule;

	// Get existing schedule object, if any
	m_activityObject.get("schedule", schedule);

	std::string smartInterval = GetSmartInterval(intervalSeconds);

	// Smart interval can only be used with no start time
	if(!smartInterval.empty()) {
		err = schedule.putString("interval", smartInterval.c_str());
		ErrorToException(err);
	} else {
		throw MailException("precise intervals not implemented", __FILE__, __LINE__);
	}

	err = m_activityObject.put("schedule", schedule);
	ErrorToException(err);
}

void ActivityBuilder::SetRequiresInternet(bool requireInternet)
{
	MojErr err = m_requirements.putBool("internet", requireInternet);
	ErrorToException(err);
}

void ActivityBuilder::SetRequiresInternetConfidence(const char* confidence)
{
	MojErr err = m_requirements.putString("internetConfidence", confidence);
	ErrorToException(err);
}

void ActivityBuilder::DisableSyncInterval()
{
	MojErr err = m_activityObject.put("schedule", false);
	ErrorToException(err);
}

void ActivityBuilder::SetPowerOptions(bool keepAwake, bool debounce)
{
	m_power = keepAwake;
	m_powerDebounce = debounce;
}

void ActivityBuilder::SetForeground(bool foreground)
{
	m_foreground = foreground;
}

void ActivityBuilder::SetImmediate(bool immediate, const std::string priority)
{
	m_immediate = immediate;
	m_priority = priority;
}

void ActivityBuilder::SetMetadata(const MojObject& metadata)
{
	MojErr err = m_activityObject.put("metadata", metadata);
	ErrorToException(err);
}

const MojObject& ActivityBuilder::GetActivityObject()
{
	MojErr err;
	MojObject type;
	
	if (m_persist) {
		err = type.put("persist", true);
		ErrorToException(err);
	}
	
	if (m_explicit) {
		err = type.put("explicit", true);
		ErrorToException(err);
	}
	
	// When you set immediate you must set a priority
	if (m_immediate && m_priority.length()) {
		err = type.put("immediate", true);
		ErrorToException(err);
		err = type.putString("priority", m_priority.c_str());
		ErrorToException(err);
	} else if (m_foreground) {
		err = type.put("foreground", true);
		ErrorToException(err);
	} else {
		err = type.put("background", true);
		ErrorToException(err);
	}

	if (m_power) {
		err = type.putBool("power", m_power);
		ErrorToException(err);
		err = type.putBool("powerDebounce", m_powerDebounce);
		ErrorToException(err);
	}

	if(!m_requirements.empty()) {
		err = m_activityObject.put("requirements", m_requirements);
	}

	err = m_activityObject.put("type", type);
	ErrorToException(err);
	
	return m_activityObject;
}
