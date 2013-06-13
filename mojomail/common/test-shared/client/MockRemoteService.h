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

#ifndef MOCKREMOTESERVICE_H_
#define MOCKREMOTESERVICE_H_

#include "CommonDefs.h"
#include "core/MojServiceRequest.h"
#include "core/MojSignal.h"
#include "core/MojObject.h"
#include <string>
#include <map>
#include <vector>

class MockRemoteService;

class MockRequest : public MojSignalHandler
{
	typedef MojServiceRequest::ReplySignal Signal;
	
public:
	MockRequest(Signal::SlotRef replySlot, const MojChar* service, const MojChar* method, const MojObject& payload);
	
	~MockRequest() {}
	
	// Set a mock service to be used for replies
	void SetService(MockRemoteService* service) { m_mockService = service; }
	
	// Reply using the service, which typically queues the response
	void Reply(MojObject& response, MojErr err);
	
	// Reply success (queued)
	void ReplySuccess();

	// Reply success with payload
	void ReplySuccess(MojObject response);

	// Reply failure (queued)
	void ReplyError(MojErr err, const char* text = NULL);

	// Reply immediately
	MojErr ReplyNow(MojObject& response, MojErr err);
	
	// Check if the request was cancelled
	bool IsCancelled() { return m_replySlot.handler() == NULL; }
	
	const std::string&	GetService() { return m_service; }
	const std::string&	GetMethod() { return m_method; }
	const MojObject&	GetPayload() { return m_payload; }
	
private:
	Signal				m_signal;
	Signal::SlotRef		m_replySlot;
	std::string			m_service;
	std::string			m_method;
	MojObject			m_payload;
	MockRemoteService*	m_mockService;
};

typedef MojRefCountedPtr<MockRequest> MockRequestPtr;

class MockReply
{
public:
	MockReply(const MockRequestPtr& req, const MojObject& payload, MojErr err)
	: m_req(req), m_payload(payload), m_err(err) {}
	
	void SendReply();
	
protected:
	MockRequestPtr	m_req;
	MojObject		m_payload;
	MojErr			m_err;
};

class MockRequestHandler
{
public:
	virtual void HandleRequest(MockRequestPtr req);
};

class MockRemoteService
{	
public:
	MockRemoteService(const char* name);
	virtual ~MockRemoteService();
	
	virtual const char* GetServiceName() { return m_serviceName; }
	
	virtual void HandleRequest(MockRequestPtr req);
	virtual void SetOffline(bool offline) { m_offline = offline; }
	virtual void Override(const char* name, MockRequestHandler& handler);
	
	virtual void Reply(const MockRequestPtr& req, const MojObject& response, MojErr err);
	
protected:
	virtual void HandleRequestImpl(MockRequestPtr req) = 0;
	virtual void SendReplies();
	virtual void SimulateOffline(MockRequestPtr req);
	
	const char* m_serviceName;
	bool m_offline;
	
	std::map<std::string, MockRequestHandler*> m_overrides;
	std::vector<MockReply> m_pendingReplies;
};

class MockPayloadRequestHandler
{
public:
	MockPayloadRequestHandler(const MojObject &response, MojErr err)
	: m_response(response), m_err(err) {}
	
	virtual void HandleRequest(MockRemoteService* serv, MockRequestPtr req)
	{
		req->Reply(m_response, m_err);
	}
protected:
	MojObject	m_response;
	MojErr		m_err;
};
#endif /*MOCKREMOTESERVICE_H_*/
