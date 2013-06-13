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

#include "data/EmailAccount.h"

const int EmailAccount::SYNC_FREQUENCY_MANUAL = 0;
const int EmailAccount::SYNC_FREQUENCY_PUSH = -1;

EmailAccount::EmailAccount()
: m_rev(0), m_syncWindowDays(7), m_syncFrequencyMins(15)
{
	m_error.errorCode = MailError::VALIDATED;
}

EmailAccount::~EmailAccount()
{
}

bool EmailAccount::IsManualSync() const
{
	return m_syncFrequencyMins == SYNC_FREQUENCY_MANUAL;
}

bool EmailAccount::IsPush() const
{
	return m_syncFrequencyMins == SYNC_FREQUENCY_PUSH;
}
