// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "commands/UpdateUidCacheCommand.h"
#include "data/UidCacheAdapter.h"

UpdateUidCacheCommand::UpdateUidCacheCommand(PopSession& session, UidCache& cache)
: PopSessionCommand(session),
  m_uidCache(cache),
  m_saveUidCacheResponseSlot(this, &UpdateUidCacheCommand::SaveUidCacheResponse)
{

}

UpdateUidCacheCommand::~UpdateUidCacheCommand()
{

}

void UpdateUidCacheCommand::RunImpl()
{
	if (m_uidCache.HasChanged()) {
		MojObject mojCache;
		UidCacheAdapter::SerializeToDatabasePopObject(m_uidCache, mojCache);
		MojObject::ObjectVec mojVec;
		mojVec.push(mojCache);

		// write old emails cache to database
		if (m_uidCache.GetId().undefined()) {
			MojLogDebug(m_log, "*** Inserting old email cache into database: %s", AsJsonString(mojCache).c_str());
			m_session.GetDatabaseInterface().AddItems(m_saveUidCacheResponseSlot, mojVec);
		} else {
			MojLogDebug(m_log, "--- Updating old email cache: %s", AsJsonString(mojCache).c_str());
			m_session.GetDatabaseInterface().UpdateItems(m_saveUidCacheResponseSlot, mojVec);
		}
	} else {
		Complete();
	}
}

MojErr UpdateUidCacheCommand::SaveUidCacheResponse(MojObject& response, MojErr err)
{
	try {
		MojLogDebug(m_log, "Save emails' UID cache response: %s", AsJsonString(response).c_str());
		// Check database response
		ErrorToException(err);

		Complete();
	} catch(const std::exception& e) {
		MojLogError(m_log, "Failed to save UID cache: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to save emails' UID cache", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}
