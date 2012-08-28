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

#ifndef POWERUSER_H_
#define POWERUSER_H_

#include "core/MojRefCount.h"
#include <string>
#include "util/Timer.h"

class PowerManager;

/**
 * PowerUser class representing one active task that needs power.
 *
 * Redundant calls to Start/Stop will be ignored. Only one "StayAwake" will be
 * registered with the power manager at any time.
 *
 * Stop() will be called when the object is destroyed.
 */
class PowerUser
{
public:
	PowerUser();
	virtual ~PowerUser();

	void Start(const MojRefCountedPtr<PowerManager>& powerManager, const std::string& reason = "");
	void Stop();

protected:
	MojRefCountedPtr<PowerManager> m_powerManager;

	bool			m_active;
	std::string		m_reason;
};

/* A PowerUser that automatically stops after some number of seconds.
 * Mainly useful to allow the device to stay awake for notifications, etc.
 */
class TemporaryPowerUser : public PowerUser
{
public:
	TemporaryPowerUser();
	virtual ~TemporaryPowerUser();

	void Start(const MojRefCountedPtr<PowerManager>& powerManager, unsigned int durationMillis = 0, const std::string& reason = "");
	void Stop();

protected:
	void Timeout();

	Timer<TemporaryPowerUser>	m_timer;
};

#endif /* POWERUSER_H_ */
