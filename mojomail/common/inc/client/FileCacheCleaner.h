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

#ifndef FILECACHECLEANER_H_
#define FILECACHECLEANER_H_

#include "client/FileCacheClient.h"
#include "core/MojSignal.h"
#include "util/Timer.h"
#include <vector>
#include <string>

class FileCacheCleaner : public MojSignalHandler
{
public:
	FileCacheCleaner(FileCacheClient& fcClient);
	virtual ~FileCacheCleaner();

	void DeletePaths(MojSignal<>::SlotRef doneSlot, const std::vector<std::string>& paths, int timeoutSeconds);

protected:
	void DeleteNextEntry();
	MojErr DeleteResponse(MojObject& response, MojErr err);

	void TimeoutExpired();

	FileCacheClient& m_fileCacheClient;
	std::vector<std::string>	m_paths;

	Timer<FileCacheCleaner>		m_timer;

	MojSignal<>			m_doneSignal;
	FileCacheClient::ReplySignal::Slot<FileCacheCleaner>	m_expireResponseSlot;
};

#endif /* FILECACHECLEANER_H_ */
