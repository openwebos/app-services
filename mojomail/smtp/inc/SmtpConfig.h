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

#ifndef SMTPCONFIG_H_
#define SMTPCONFIG_H_

#include "core/MojCoreDefs.h"
#include <string>

class SmtpConfig
{
public:
	SmtpConfig();
	virtual ~SmtpConfig();

	// Get global instance
	static SmtpConfig& GetConfig() { return s_instance; }

	// Parse config
	MojErr ParseConfig(const MojObject& config);

	int GetInactivityTimeout() const { return m_inactivityTimeout; }
	void SetInactivityTimeout(int timeout) { m_inactivityTimeout = timeout; }

protected:
	void GetOptionalInt(const MojObject& obj, const char* prop, int& value, int minValue, int maxValue);

	static MojLogger& s_log;
	static SmtpConfig s_instance;

	// Number of seconds before the process should shut down if nothing is active.
	// Zero for no timeout
	int	m_inactivityTimeout;
};

#endif /* SMTPCONFIG_H_ */
