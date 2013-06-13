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

#include "client/FileCacheCleaner.h"
#include "util/LogUtils.h"

using namespace std;

FileCacheCleaner::FileCacheCleaner(FileCacheClient& fcClient)
: m_fileCacheClient(fcClient),
  m_doneSignal(this),
  m_expireResponseSlot(this, &FileCacheCleaner::DeleteResponse)
{
}

FileCacheCleaner::~FileCacheCleaner()
{
}

void FileCacheCleaner::DeletePaths(MojSignal<>::SlotRef doneSlot, const vector<string>& paths, int timeoutSeconds)
{
	m_doneSignal.connect(doneSlot);

	m_paths = paths;

	if (timeoutSeconds > 0) {
		m_timer.SetTimeout(timeoutSeconds, this, &FileCacheCleaner::TimeoutExpired);
	}

	DeleteNextEntry();
}

void FileCacheCleaner::DeleteNextEntry()
{
	m_expireResponseSlot.cancel();

	if (!m_paths.empty()) {
		string path = m_paths.back();
		m_paths.pop_back();

		m_fileCacheClient.ExpireCacheObject(m_expireResponseSlot, path.c_str());
	} else {
		// Done
		m_timer.Cancel();
		m_doneSignal.fire();
	}
}

MojErr FileCacheCleaner::DeleteResponse(MojObject& response, MojErr err)
{
	if (err) {
		MojLogWarning(LogUtils::s_commonLog, "error deleting path: %d", err);
	}

	DeleteNextEntry();

	return MojErrNone;
}

void FileCacheCleaner::TimeoutExpired()
{
	MojLogWarning(LogUtils::s_commonLog, "filecache expire request timed out");

	// This cancels the current request and calls the done signal
	m_expireResponseSlot.cancel();

	m_doneSignal.fire();
}
