#include "MockRemoteService.h"
#include "core/MojObject.h"
#include "CommonPrivate.h"

MockRemoteService::MockRemoteService(const char* name)
: m_serviceName(name), m_offline(false) 
{
}

MockRemoteService::~MockRemoteService()
{
}

MockRequest::MockRequest(Signal::SlotRef replySlot, const MojChar* service, const MojChar* method, const MojObject& payload)
: m_signal(this), m_replySlot(replySlot), m_service(service), m_method(method), m_payload(payload), m_mockService(NULL)
{
	m_signal.connect(m_replySlot);
}

void MockRequest::Reply(MojObject& response, MojErr err)
{
	if(m_mockService)
		m_mockService->Reply(this, response, err);
	else
		ReplyNow(response, err);
}

void MockRequest::ReplySuccess()
{
	MojObject empty;
	ReplySuccess(empty);
}

void MockRequest::ReplySuccess(MojObject response)
{
	MojErr err = response.put("returnValue", true);
	ErrorToException(err);

	Reply(response, MojErrNone);
}

void MockRequest::ReplyError(MojErr errorCode, const char* errorText)
{
	MojObject response;

	MojErr err = response.put("errorCode", errorCode);
	ErrorToException(err);

	if (errorText) {
		err = response.putString("errorText", errorText);
		ErrorToException(err);
	}

	Reply(response, MojErrNone);
}

MojErr MockRequest::ReplyNow(MojObject& response, MojErr err)
{
	return m_signal.call(response, err);
}

void MockReply::SendReply()
{
	m_req->ReplyNow(m_payload, m_err);
}
	
void MockRemoteService::HandleRequest(MockRequestPtr req)
{
	req->SetService(this);
	
	if(m_offline) {
		SimulateOffline(req);
		return;
	}
	
	// Check if there's any overrides registered
	std::map<std::string, MockRequestHandler *>::iterator it = m_overrides.find(req->GetMethod());
	if(it != m_overrides.end()) {
		MockRequestHandler *handler = it->second;
		if(handler) {
			handler->HandleRequest(req);
			return;
		}
	}
	
	HandleRequestImpl(req);
	SendReplies();
}

void MockRemoteService::Reply(const MockRequestPtr& req, const MojObject& response, MojErr err)
{
	m_pendingReplies.push_back( MockReply(req, response, err) );
}

void MockRemoteService::SendReplies()
{
	// Work with a local copy
	std::vector<MockReply> replies = m_pendingReplies;
	m_pendingReplies.clear();
	
	std::vector<MockReply>::iterator it;
	for(it = replies.begin(); it != replies.end(); ++it) {
		MockReply& reply = *it;
		reply.SendReply();
	}
}

void MockRemoteService::SimulateOffline(MockRequestPtr req)
{
	MojErr err;
	MojObject payload;

	err = payload.put("returnValue", false);
	ErrorToException(err);
	err = payload.put("errorCode", -1);
	ErrorToException(err);

	req->ReplyNow(payload, MojErrInternal);
}

void MockRemoteService::Override(const char* name, MockRequestHandler& handler)
{
	m_overrides[name] = &handler;
}
