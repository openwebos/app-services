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

#ifndef MOCKBUSCLIENT_H_
#define MOCKBUSCLIENT_H_

#include "client/BusClient.h"
#include "client/MockRemoteService.h"
#include <string>
#include <map>
#include <deque>

class MockBusClient : public BusClient
{
public:
	MockBusClient();
	virtual ~MockBusClient();
	
	virtual void SendRequest(MojServiceRequest::ReplySignal::SlotRef handler, const MojChar* service,
			const MojChar* method, const MojObject& payload, MojUInt32 numReplies);
	
	virtual void AddMockService(MockRemoteService& mockService);
	
	void PrintRequests(const std::string& desc = "current");

	MockRequestPtr GetLastRequest() { return m_requests.back(); }
	
	MockRequestPtr PopLastRequest()
	{
		MockRequestPtr lastRequest = m_requests.back();
		m_requests.pop_back();
		return lastRequest;
	}

	void ProcessRequests();

protected:
	std::map<std::string, MockRemoteService*> m_services;
	std::deque<MockRequestPtr> m_requests;
};

#endif /*MOCKBUSCLIENT_H_*/
