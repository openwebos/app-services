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

#ifndef HANDLEREQUESTCOMMAND_H_
#define HANDLEREQUESTCOMMAND_H_

#include <queue>
#include "client/PopSession.h"
#include "commands/PopSessionPowerCommand.h"
#include "request/Request.h"

class HandleRequestCommand :  public PopSessionPowerCommand
{
public:
	HandleRequestCommand(PopSession& session, const std::string& awakeReason);
	virtual ~HandleRequestCommand();

	void 			QueueRequest(Request::RequestPtr request);
protected:
	typedef std::queue<Request::RequestPtr> RequestQueue;

	virtual void 	HandleNextRequest() = 0;
	virtual MojErr 	HandleRequestResponse() = 0;

	RequestQueue	m_requests;
	MojSignal<>::Slot<HandleRequestCommand>		m_handleRequestResponseSlot;
};

#endif /* HANDLEREQUESTCOMMAND_H_ */
