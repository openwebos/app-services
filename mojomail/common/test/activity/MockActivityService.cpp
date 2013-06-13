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

#include "activity/MockActivityService.h"
#include "CommonPrivate.h"

MockActivityService::MockActivityService()
: MockRemoteService("com.palm.activitymanager")
{
}

MockActivityService::~MockActivityService()
{
}

void MockActivityService::HandleRequestImpl(MockRequestPtr req)
{
	const std::string& method = req->GetMethod();
	if(method == "adopt") {
		Adopt(req);
	} else if(method == "complete") {
		Complete(req);
	} else {
		throw MailException("Unknown activity manager method", __FILE__, __LINE__);
	}
}

void MockActivityService::Adopt(MockRequestPtr req)
{
	MojErr err;
	MojObject adoptReply;

	err = adoptReply.put("adopted", true);
	ErrorToException(err);
	err = adoptReply.put("returnValue", true);
	ErrorToException(err);

	req->Reply(adoptReply, MojErrNone);
}

void MockActivityService::Complete(MockRequestPtr req)
{
	// TODO
	MojErr err;
	MojObject completeReply;

	err = completeReply.put("returnValue", true);
	ErrorToException(err);

	req->Reply(completeReply, MojErrNone);
}
