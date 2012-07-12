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

#ifndef BUSCLIENT_H_
#define BUSCLIENT_H_

#include "CommonDefs.h"
#include "core/MojServiceRequest.h"
#include <string>

class BusClient
{
public:
	typedef MojServiceRequest::ReplySignal ReplySignal;

	BusClient(MojService* service, const std::string& serviceName = "");
	virtual ~BusClient();

	/**
	 * @return ref counted pointer to a MojServiceRequest used for sending requests over the bus
	 */
	virtual MojRefCountedPtr<MojServiceRequest> CreateRequest();
	
	virtual void SendRequest(MojServiceRequest::ReplySignal::SlotRef handler, const MojChar* service, const MojChar* method,
			const MojObject& payload, MojUInt32 numReplies = 1);
	
	const std::string& GetServiceName() const { return m_serviceName; }

protected:
	MojService*		m_service;
	std::string		m_serviceName;
};

#endif /*BUSCLIENT_H_*/
