// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef REQUEST_H_
#define REQUEST_H_

#include <vector>
#include <boost/shared_ptr.hpp>
#include "client/DownloadListener.h"
#include "core/MojObject.h"
#include "core/MojSignal.h"

class Request : public MojSignalHandler
{
public:
	typedef enum {
		Request_Fetch_Email,
		Request_Delete_Email,
		Reqeuest_Move_Email
	} RequestType;

	typedef enum {
		Priority_High,
		Priority_Low
	} RequestPriority;

	typedef MojRefCountedPtr<Request> 			RequestPtr;
	typedef std::vector<Request::RequestPtr> 	RequestPtrVector;

	Request(RequestType type, const MojObject& emailId, const MojObject& partId = MojObject::Null);
	~Request();

	void  SetDownloadListener(boost::shared_ptr<DownloadListener>& listener) { m_listener = listener; }
	void  SetPriority(RequestPriority priority) { m_priority = priority; }
	void  ConnectDoneSlot(MojSignal<>::SlotRef doneSlot);
	void  Done();

	const RequestType 	GetRequestType() const	{ return m_type; }
	const MojObject&	GetEmailId() const		{ return m_emailId; }
	const MojObject&	GetPartId() const		{ return m_partId; }
	RequestPriority		GetPriority() const		{ return m_priority; }
	const boost::shared_ptr<DownloadListener>&	GetDownloadListener() const	 { return m_listener; }
private:
	RequestType 								m_type;
	MojObject									m_emailId;
	MojObject									m_partId;	// optional parameter
	boost::shared_ptr<DownloadListener>			m_listener;	// optional parameter
	RequestPriority								m_priority;
	MojSignal<>									m_doneSignal;
};

#endif /* REQUEST_H_ */
