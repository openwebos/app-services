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

#ifndef DBMERGEPROXY_H_
#define DBMERGEPROXY_H_

#include "db/MojDbServiceClient.h"

/* This class is a hack to work around DFISH-7445. It can be used in place of a normal
 * MojDbServiceClient but retries any merge requests that return MojErrDbInconsistentIndex.
 */
class DbMergeProxy : public MojDbServiceClient
{
public:
	DbMergeProxy(MojService* service, const MojChar* serviceName = MojDbServiceDefs::ServiceName);

	virtual MojErr merge(Signal::SlotRef handler, const MojObject* begin,
						 const MojObject* end, MojUInt32 flags = MojDb::FlagNone);
	virtual MojErr merge(Signal::SlotRef handler, const MojDbQuery& query,
						 const MojObject& props, MojUInt32 flags = MojDb::FlagNone);

	MojErr realMerge(Signal::SlotRef handler, const MojObject* begin,
						 const MojObject* end, MojUInt32 flags = MojDb::FlagNone);
	MojErr realMerge(Signal::SlotRef handler, const MojDbQuery& query,
						 const MojObject& props, MojUInt32 flags = MojDb::FlagNone);
};

#endif /* DBMERGEPROXY_H_ */
