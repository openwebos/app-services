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

#include "client/DbMergeProxy.h"
#include "util/LogUtils.h"
#include "CommonPrivate.h"

class DbMergeRequest : public MojSignalHandler
{
public:
	DbMergeRequest(DbMergeProxy& client, MojDbClient::Signal::SlotRef appSlot, MojUInt32 flags);
	virtual ~DbMergeRequest();

protected:
	virtual MojErr execute() = 0;
	MojErr handleResponse(MojObject& response, MojErr err);
	MojErr handleCancel();

	DbMergeProxy&		m_dbClient;
	MojUInt32			m_flags;
	int					m_retries;

	static int			s_numLiveRequests; // to check for memory leaks

	MojDbClient::Signal							m_signal;	// sends responses to the app
	MojDbClient::Signal::Slot<DbMergeRequest>	m_dbSlot;	// receives responses from the db
};

int DbMergeRequest::s_numLiveRequests = 0;

DbMergeRequest::DbMergeRequest(DbMergeProxy& dbClient, MojDbClient::Signal::SlotRef appSlot, MojUInt32 flags)
: m_dbClient(dbClient),
  m_flags(flags),
  m_retries(0),
  m_signal(this),
  m_dbSlot(this, &DbMergeRequest::handleResponse)
{
	m_signal.connect(appSlot);

	++s_numLiveRequests;
}

DbMergeRequest::~DbMergeRequest()
{
	--s_numLiveRequests;

	//fprintf(stderr, "requests remaining: %d\n", s_numLiveRequests);
}


MojErr DbMergeRequest::handleCancel()
{
	m_dbSlot.cancel();
	return MojErrNone;
}

MojErr DbMergeRequest::handleResponse(MojObject& response, MojErr err)
{
	if (err != MojErrDbInconsistentIndex || m_retries > 2) {
		return m_signal.fire(response, err);
	} else {
		// Uh oh -- retry
		MojLogError(LogUtils::s_commonLog, "db merge failed for no good reason (DFISH-7445); retrying");

		m_dbSlot.cancel();

		m_retries++;
		return execute();
	}
}

class DbMergeArrayRequest : public DbMergeRequest
{
public:
	DbMergeArrayRequest(DbMergeProxy& dbClient, MojDbClient::Signal::SlotRef slot, const MojObject* begin, const MojObject* end, MojUInt32 flags = MojDb::FlagNone)
	: DbMergeRequest(dbClient, slot, flags)
	{
		MojErr err = m_array.append(begin, end);
		ErrorToException(err);
	}
	virtual ~DbMergeArrayRequest() {}

	MojErr execute()
	{
		return m_dbClient.realMerge(m_dbSlot, m_array.begin(), m_array.end(), m_flags);
	}

protected:
	MojObject::ObjectVec	m_array;
};

class DbMergePropsRequest : public DbMergeRequest
{
public:
	DbMergePropsRequest(DbMergeProxy& dbClient, MojDbClient::Signal::SlotRef slot, const MojDbQuery& query, const MojObject& props, MojUInt32 flags = MojDb::FlagNone)
	: DbMergeRequest(dbClient, slot, flags),
	  m_query(query),
	  m_props(props)
	{
	}
	virtual ~DbMergePropsRequest() {}

	MojErr execute()
	{
		return m_dbClient.realMerge(m_dbSlot, m_query, m_props, m_flags);
	}

protected:
	MojDbQuery	m_query;
	MojObject	m_props;
};

DbMergeProxy::DbMergeProxy(MojService* service, const MojChar* serviceName)
: MojDbServiceClient(service, serviceName)
{
}

MojErr DbMergeProxy::merge(Signal::SlotRef handler, const MojObject* begin, const MojObject* end, MojUInt32 flags)
{
	MojRefCountedPtr<DbMergeArrayRequest> request(new DbMergeArrayRequest(*this, handler, begin, end, flags));
	return request->execute();
}

MojErr DbMergeProxy::merge(Signal::SlotRef handler, const MojDbQuery& query, const MojObject& props, MojUInt32 flags)
{
	MojRefCountedPtr<DbMergePropsRequest> request(new DbMergePropsRequest(*this, handler, query, props, flags));
	return request->execute();
}

// Call superclass merge
MojErr DbMergeProxy::realMerge(Signal::SlotRef handler, const MojObject* begin, const MojObject* end, MojUInt32 flags)
{
	return MojDbServiceClient::merge(handler, begin, end, flags);
}

// Call superclass merge
MojErr DbMergeProxy::realMerge(Signal::SlotRef handler, const MojDbQuery& query, const MojObject& props, MojUInt32 flags)
{
	return MojDbServiceClient::merge(handler, query, props, flags);
}
