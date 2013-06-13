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

#ifndef ACTIVITYPARSER_H_
#define ACTIVITYPARSER_H_

#include "CommonDefs.h"
#include "core/MojObject.h"

class Activity;

class ActivityParser
{
public:
	ActivityParser();
	virtual ~ActivityParser();
	
	// Parse a payload and extract the $activity, if any.
	// Will throw an exception if there was a trigger with a returnValue false.
	void ParseCallbackPayload(const MojObject& payload);
	
	bool HasActivity() const;
	
	const MojObject& GetActivityId() const;

	const MojString& GetActivityName() const;
	
	const MojObject& GetActivityInfo() const;
	
	MojRefCountedPtr<Activity> GetActivity() const;

	static MojRefCountedPtr<Activity> GetActivityFromPayload(const MojObject& payload);

protected:
	MojObject	m_activityId;
	MojString	m_activityName;
	MojObject	m_activityInfo; // holds contents of $activity
	MojObject	m_serial;
	MojObject	m_creator;
};

#endif /*ACTIVITYPARSER_H_*/
