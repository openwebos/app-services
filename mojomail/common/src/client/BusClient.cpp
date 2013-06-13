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

#include "client/BusClient.h"
#include "CommonPrivate.h"
#include "core/MojService.h"
#include "core/MojLogEngine.h"

BusClient::BusClient(MojService* service, const std::string& serviceName)
: m_service(service), m_serviceName(serviceName)
{
}

BusClient::~BusClient()
{
}

MojRefCountedPtr<MojServiceRequest> BusClient::CreateRequest()
{
	MojRefCountedPtr<MojServiceRequest> req;
	MojErr err = m_service->createRequest(req);
	ErrorToException(err);
	return req;
}

void BusClient::SendRequest(MojServiceRequest::ReplySignal::SlotRef handler, const MojChar* service,
		const MojChar* method, const MojObject& payload, MojUInt32 numReplies)
{
	MojRefCountedPtr<MojServiceRequest> req = CreateRequest();

	MojErr err = req->send(handler, service, method, payload, numReplies);
	ErrorToException(err);
}
