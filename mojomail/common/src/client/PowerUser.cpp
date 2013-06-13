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

#include "client/PowerUser.h"
#include "client/PowerManager.h"
#include "exceptions/MailException.h"

PowerUser::PowerUser()
: m_active(false)
{
}

PowerUser::~PowerUser()
{
	if (m_active) {
		MojLogWarning(PowerManager::s_log, "power user still active at destruction: %s", m_reason.c_str());
		Stop();
	}
}

void PowerUser::Start(const MojRefCountedPtr<PowerManager>& powerManager, const std::string& reason)
{
	m_reason = reason;

	if(m_powerManager != powerManager && m_powerManager.get() != NULL) {
		throw MailException("can't use a different power manager", __FILE__, __LINE__);
	}

	m_powerManager = powerManager;

	if(m_powerManager.get() && !m_active) {
		m_powerManager->StayAwake(true, m_reason);
		m_active = true;
	}
}

void PowerUser::Stop()
{
	if(m_powerManager.get() && m_active) {
		m_powerManager->StayAwake(false, m_reason);
		m_active = false;
	}

	m_powerManager.reset();
}

TemporaryPowerUser::TemporaryPowerUser()
{
}

TemporaryPowerUser::~TemporaryPowerUser()
{
}

void TemporaryPowerUser::Start(const MojRefCountedPtr<PowerManager>& powerManager, unsigned int durationMillis, const std::string& reason)
{
	PowerUser::Start(powerManager, reason);

	if (durationMillis > 0) {
		m_timer.SetTimeoutMillis(durationMillis, this, &TemporaryPowerUser::Timeout);
	}
}

void TemporaryPowerUser::Stop()
{
	m_timer.Cancel();

	PowerUser::Stop();
}

void TemporaryPowerUser::Timeout()
{
	PowerUser::Stop();
}
