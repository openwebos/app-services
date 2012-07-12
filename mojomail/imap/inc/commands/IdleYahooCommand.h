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

#ifndef IDLEYAHOOCOMMAND_H_
#define IDLEYAHOOCOMMAND_H_
//#include "commands/SyncFolderCommand.h"
#include "commands/BaseIdleCommand.h"
#include "core/MojObject.h"
#include "util/Timer.h"

class SyncEmailsCommand;

class IdleYahooCommand : public BaseIdleCommand
{
public:
	IdleYahooCommand(ImapSession& session, const MojObject& folderId);
	virtual ~IdleYahooCommand();

	void RunImpl();

	// Unsubscribe the yahoo service.
	void EndIdle();

	void Status(MojObject& status) const;

	std::string Describe() const;

protected:

	void InitialSync();
	MojErr SyncDone();

	void SubscribeWakeup();
	void SubscribeTimeout();
	MojErr SubscriptionResponse(MojObject& response, MojErr err);

	virtual void Failure(const std::exception& e);

	void Cleanup();

	bool 		m_subscribedYahoo;
	bool 		m_syncInProgress;
	MojObject	m_folderId;

	static const int SERVICE_RESPONSE_TIMEOUT;

	Timer<IdleYahooCommand>		m_subscribeTimer;

	MojRefCountedPtr<SyncEmailsCommand>		m_syncCommand;

	MojSignal<>::Slot<IdleYahooCommand>							m_syncSlot;
	MojServiceRequest::ReplySignal::Slot<IdleYahooCommand>		m_subscriptionResponseSlot;
};

#endif /* IDLEYAHOOCOMMAND_H_ */
