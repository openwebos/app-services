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

#include "commands/LoadUidCacheCommand.h"
#include "data/UidCacheAdapter.h"

LoadUidCacheCommand::LoadUidCacheCommand(PopSession& session, const MojObject& accountId, UidCache& cache)
: PopSessionCommand(session),
  m_accountId(accountId),
  m_uidCache(cache),
  m_getUidCacheResponseSlot(this, &LoadUidCacheCommand::GetUidCacheResponse)
{

}

LoadUidCacheCommand::~LoadUidCacheCommand()
{

}

void LoadUidCacheCommand::RunImpl()
{
	CommandTraceFunction();
	m_session.GetDatabaseInterface().GetUidCache(m_getUidCacheResponseSlot, m_accountId);
}

MojErr LoadUidCacheCommand::GetUidCacheResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	try {
		// Check database response
		ErrorToException(err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		MojObject::ArrayIterator itr;
		err = results.arrayBegin(itr);
		ErrorToException(err);

		if (itr == results.arrayEnd()) {
			// old emails cache is empty
			m_uidCache.SetAccountId(m_accountId);
		} else {
			UidCacheAdapter::ParseDatabasePopObject(*itr, m_uidCache);
		}

		Complete();
	} catch(const std::exception& e) {
		MojLogError(m_log, "Failed to load UID cache: %s", e.what());
		Failure(e);
	} catch(...) {
		MojLogError(m_log, "Uncaught exception %s", __PRETTY_FUNCTION__);
		MailException exc("Unable to get emails' UID cache", __FILE__, __LINE__);
		Failure(exc);
	}

	return MojErrNone;
}
