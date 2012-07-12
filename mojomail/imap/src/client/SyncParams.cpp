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

#include "client/SyncParams.h"
#include "ImapPrivate.h"

SyncParams::SyncParams()
: m_params(new Params())
{
}

SyncParams::~SyncParams()
{
}

std::vector<ActivityPtr> SyncParams::GetAllActivities() const
{
	std::vector<ActivityPtr> all;

	all.insert(all.end(), m_params->syncActivities.begin(), m_params->syncActivities.end());
	all.insert(all.end(), m_params->connectionActivities.begin(), m_params->connectionActivities.end());

	return all;
}

void SyncParams::Status(MojObject& status) const
{
	MojErr err;

	err = status.put("force", GetForce());
	ErrorToException(err);

	if(!GetReason().empty()) {
		err = status.putString("reason", GetReason().c_str());
		ErrorToException(err);
	}

	if(!GetBindAddress().empty()) {
		err = status.putString("bindAddress", GetBindAddress().c_str());
		ErrorToException(err);
	}
}
