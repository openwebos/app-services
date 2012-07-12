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

#include "SmtpConfig.h"
#include "SmtpCommon.h"

SmtpConfig SmtpConfig::s_instance;

SmtpConfig::SmtpConfig()
: m_inactivityTimeout(30) // 30 seconds
{
}

SmtpConfig::~SmtpConfig()
{
}

void SmtpConfig::GetOptionalInt(const MojObject& obj, const char* prop, int& value, int minValue, int maxValue)
{
	bool hasProp = false;
	int temp = value;

	MojErr err = obj.get(prop, temp, hasProp);
	ErrorToException(err);

	if(hasProp) {
		value = std::max(minValue, std::min(temp, maxValue));
	}
}

MojErr SmtpConfig::ParseConfig(const MojObject& conf)
{
	GetOptionalInt(conf, "inactivityTimeoutSeconds", m_inactivityTimeout, 0, MojInt32Max);

	return MojErrNone;
}
