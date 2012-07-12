// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef UPDATEUIDCACHECOMMAND_H_
#define UPDATEUIDCACHECOMMAND_H_

#include "commands/PopSessionCommand.h"
#include "data/UidCache.h"

class UpdateUidCacheCommand : public PopSessionCommand
{
public:
	UpdateUidCacheCommand(PopSession& session, UidCache& cache);
	~UpdateUidCacheCommand();

	void 	RunImpl();
	MojErr	SaveUidCacheResponse(MojObject& response, MojErr err);
private:
	UidCache&											m_uidCache;
	MojDbClient::Signal::Slot<UpdateUidCacheCommand> 	m_saveUidCacheResponseSlot;
};

#endif /* UPDATEUIDCACHECOMMAND_H_ */
