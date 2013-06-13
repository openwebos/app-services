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

#include "activity/Activity.h"
#include "client/MockBusClient.h"
#include "core/MojSignal.h"
#include <string>
#include "activity/MockActivityService.h"
#include <gtest/gtest.h>
#include "CommonPrivate.h"

class MockActivitySlots : public MojSignalHandler
{
public:
	MockActivitySlots(ActivityPtr activity)
	: m_expect(false), m_expectError(false), m_expectUpdate(false),
	  m_updateSlot(this, &MockActivitySlots::Update),
	  m_errorSlot(this, &MockActivitySlots::Error)
	{
		activity->SetSlots(m_updateSlot, m_errorSlot);
	}
	
	void ExpectUpdate(Activity::EventType type)
	{
		m_expect = true;
		m_expectError = false;
		m_expectUpdate = true;
		m_expectEventType = type;
	}
	
	void ExpectError()
	{
		m_expect = true;
		m_expectError = true;
		m_expectUpdate = false;
	}
	
	// Check that there's no expected error/update that didn't get called
	void Check()
	{
		if(m_expect) {
			ASSERT_FALSE(m_expectError);
			ASSERT_FALSE(m_expectUpdate);
		}
		
		m_expect = false;
		m_expectError = false;
		m_expectUpdate = false;
	}
	
protected:
	virtual ~MockActivitySlots() {}
	
	MojErr Update(Activity* activity, Activity::EventType event)
	{
		if(m_expectUpdate && event == m_expectEventType) {
			m_expectUpdate = false;
		} else if(m_expect) {
			throw MailException("Unexpected update", __FILE__, __LINE__);
		}
		return MojErrNone;
	}
	
	MojErr Error(Activity*, Activity::ErrorType error, const std::exception& e)
	{
		if(m_expectError) {
			m_expectError = false;
		} else if(m_expect) {
			throw MailException("Unexpected error", __FILE__, __LINE__);
		}
		return MojErrNone;
	}
	
	bool					m_expect;
	bool					m_expectError;
	bool					m_expectUpdate;
	Activity::EventType		m_expectEventType;
	
	Activity::UpdateSignal::Slot<MockActivitySlots>	m_updateSlot;
	Activity::ErrorSignal::Slot<MockActivitySlots>	m_errorSlot;
};

void CheckRequest(MockRequestPtr req, const char* service, const char* method)
{
	ASSERT_EQ( service, req->GetService() );
	ASSERT_EQ( method, req->GetMethod() );
}

MojInt32 GetActivityId(const MojObject& payload)
{
	MojInt32 reqActivityId;
	MojErr err = payload.getRequired("activityId", reqActivityId);
	ErrorToException(err);
	return reqActivityId;
}

TEST(ActivityTest, TestAdopt)
{
	MockActivityService		mockActivityService;
	MockBusClient			serviceClient;
	
	MojInt32 activityId = 1;
	ActivityPtr activity = Activity::PrepareAdoptedActivity(activityId);
	
	ASSERT_TRUE( activity->CanAdopt() );
	
	activity->Adopt(serviceClient);
	
	ASSERT_TRUE( !activity->CanAdopt() );
	
	// Check adopt response
	MockRequestPtr req = serviceClient.GetLastRequest();
	CheckRequest(req, "com.palm.activitymanager", "adopt");
	ASSERT_EQ( activityId, GetActivityId(req->GetPayload()) );
	
	// Reply to adopt
	mockActivityService.HandleRequest(req);
	
	ASSERT_TRUE( activity->IsActive() );
	
	activity->Complete(serviceClient);
}

TEST(ActivityTest, TestAdoptFailure)
{
	MockActivityService		mockActivityService;
	MockBusClient			serviceClient;

	MojInt32 activityId = 1;
	ActivityPtr activity = Activity::PrepareAdoptedActivity(activityId);
	MojRefCountedPtr<MockActivitySlots> slots(new MockActivitySlots(activity));
	
	activity->Adopt(serviceClient);
	MockRequestPtr req = serviceClient.GetLastRequest();
	
	// Should handle the error and pass it along to the error slot
	slots->ExpectError();
	MojObject response;
	req->ReplyNow(response, MojErrInternal);
	slots->Check();
}

TEST(ActivityTest, TestActivityManagerOffline)
{
	MockActivityService		mockActivityService;
	MockBusClient			serviceClient;

	mockActivityService.SetOffline(true);

	MojInt32 activityId = 1;
	ActivityPtr activity = Activity::PrepareAdoptedActivity(activityId);
	MojRefCountedPtr<MockActivitySlots> slots(new MockActivitySlots(activity));
	
	activity->Adopt(serviceClient);
	MockRequestPtr req = serviceClient.GetLastRequest();
	
	// Should handle the error and pass it along to the error slot
	slots->ExpectError();
	mockActivityService.HandleRequest(req);
	slots->Check();
}
