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

#ifndef ACTIVITYBUILDER_H_
#define ACTIVITYBUILDER_H_

#include <string>
#include "CommonDefs.h"
#include "db/MojDbClient.h"

/**
 * Representation of an Activity in ActivityManager
 */
class ActivityBuilder
{
public:
	static const char* const PRIORITY_LOW;
	static const char* const PRIORITY_NORMAL;
	static const char* const PRIORITY_HIGH;

	ActivityBuilder();
	virtual ~ActivityBuilder();
	
	void SetName(const char* name);
	void SetDescription(const char* desc);
	// Persistent activities are preserved across reboots.
	void SetPersist(bool persist);
	// Explicit activities must be explicitly completed or stopped/cancelled,
	// or else activity manager will retry running the activity.
	void SetExplicit(bool explicitActivity);
	void SetCallback(const char* uri, const MojObject& params);
	void SetDatabaseWatchTrigger(const MojDbQuery& query);
	void DisableDatabaseWatchTrigger();

	// Set and offset and interval. Either value can be zero.
	void SetSyncInterval(int startOffsetSeconds, int intervalSeconds);

	void SetStartDelay(int delaySeconds);
	void SetSmartInterval(int intervalSeconds);

	void DisableSyncInterval();
	void SetPowerOptions(bool keepAwake, bool debounce);
	void SetForeground(bool foreground);
	void SetImmediate(bool immediate, const std::string priority);
	void SetRequiresInternet(bool requireInternet);
	void SetRequiresInternetConfidence(const char* confidence);
	void SetMetadata(const MojObject& metadata);
	
	std::string GetSmartInterval(int seconds);

	MojString GetName();
	const MojObject& GetActivityObject();

	MojObject		m_activityObject;
	bool			m_persist;
	bool			m_explicit;
	bool			m_foreground;
	bool			m_immediate;
	bool			m_power;
	bool			m_powerDebounce;
	MojObject		m_requirements;
	std::string		m_priority;
};

#endif /*ACTIVITYBUILDER_H_*/
