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

#ifndef IMAPSYNCSESSIONCOMMAND_H_
#define IMAPSYNCSESSIONCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "core/MojObject.h"

class SyncSession;

class ImapSyncSessionCommand : public ImapSessionCommand
{
public:
	ImapSyncSessionCommand(ImapSession& session, const MojObject& folderId, Priority priority = NormalPriority);
	virtual ~ImapSyncSessionCommand();

	virtual void SetSyncSession(const MojRefCountedPtr<SyncSession>& syncSession);

	virtual MojErr SyncSessionReady();

	virtual void Cleanup();
	virtual void Complete();

	virtual void Failure(const std::exception& e);

protected:
	virtual bool PrepareToRun();
	MojInt64 GetLastSyncRev() const;

	MojRefCountedPtr<SyncSession>	m_syncSession;
	MojObject						m_folderId;

private:
	bool	m_syncSessionReady;
	MojSignal<>::Slot<ImapSyncSessionCommand>	m_syncSessionReadySlot;
};

#endif /* IMAPSYNCSESSIONCOMMAND_H_ */
