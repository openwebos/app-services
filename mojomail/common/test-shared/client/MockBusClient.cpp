#include "client/MockBusClient.h"
#include "client/MockRemoteService.h"
#include <boost/foreach.hpp>
#include "CommonPrivate.h"
#include <iostream>

using namespace std;

MockBusClient::MockBusClient()
: BusClient(NULL)
{
}
	
MockBusClient::~MockBusClient()
{
}
	
void MockBusClient::SendRequest(MojServiceRequest::ReplySignal::SlotRef handler, const MojChar* service,
		const MojChar* method, const MojObject& payload, MojUInt32 numReplies)
{
	MockRequestPtr req(new MockRequest(handler, service, method, payload));
	m_requests.push_back(req);
}

void MockBusClient::AddMockService(MockRemoteService& mockService)
{
	const char* name = mockService.GetServiceName();
	m_services[name] = &mockService;
}

void MockBusClient::PrintRequests(const string& desc)
{
	cerr << "## " << desc << " requests: " << m_requests.size() << " ##" << endl;
	BOOST_FOREACH (const MockRequestPtr& req, m_requests) {
		cerr << "Request: " << req->GetService() << "/" << req->GetMethod() << ": " << AsJsonString(req->GetPayload()) << endl;
	}
}

void MockBusClient::ProcessRequests()
{
	while(!m_requests.empty()) {
		MockRequestPtr req = m_requests.front();
		m_requests.pop_front();

		if(m_services.find(req->GetService()) != m_services.end()) {
			MockRemoteService* service = m_services[req->GetService()];
			service->HandleRequest(req);
		}
	}
}
