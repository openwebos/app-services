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
#include "activity/ActivitySet.h"
#include "activity/MockActivityService.h"
#include "client/MockBusClient.h"
#include "MockDoneSlot.h"
#include <gtest/gtest.h>

void Start(MockBusClient& busClient, MojRefCountedPtr<ActivitySet>& actSet)
{
	MockDoneSlot startSlot;
	actSet->StartActivities(startSlot.GetSlot());
	busClient.ProcessRequests();

	ASSERT_TRUE( startSlot.Called() );
}

void End(MockBusClient& busClient, MojRefCountedPtr<ActivitySet>& actSet)
{
	MockDoneSlot endSlot;
	actSet->EndActivities(endSlot.GetSlot());
	busClient.ProcessRequests();

	ASSERT_TRUE( endSlot.Called() );
}

TEST(ActivitySetTest, TestStart)
{
	MockActivityService		service;
	MockBusClient			busClient;

	busClient.AddMockService(service);

	ActivityPtr activity = Activity::PrepareAdoptedActivity(1);

	MojRefCountedPtr<ActivitySet> actSet(new ActivitySet(busClient));
	actSet->AddActivity(activity);

	Start(busClient, actSet);
	ASSERT_TRUE( activity->IsActive() );
}

TEST(ActivitySetTest, TestAlreadyStarted)
{
	MockActivityService		service;
	MockBusClient			busClient;

	busClient.AddMockService(service);

	ActivityPtr activity = Activity::PrepareAdoptedActivity(1);

	MojRefCountedPtr<ActivitySet> actSet(new ActivitySet(busClient));
	actSet->AddActivity(activity);

	Start(busClient, actSet);

	// Try to start it again
	Start(busClient, actSet);
}

TEST(ActivitySetTest, TestEnd)
{
	MockActivityService		service;
	MockBusClient			busClient;

	busClient.AddMockService(service);

	ActivityPtr activity = Activity::PrepareAdoptedActivity(1);

	MojRefCountedPtr<ActivitySet> actSet(new ActivitySet(busClient));
	actSet->AddActivity(activity);

	Start(busClient, actSet);
	End(busClient, actSet);
}

TEST(ActivitySetTest, TestEndOrder)
{
	MockActivityService		service;
	MockBusClient			busClient;

	busClient.AddMockService(service);
	MojRefCountedPtr<ActivitySet> actSet(new ActivitySet(busClient));

	ActivityPtr activity1 = Activity::PrepareAdoptedActivity(1);
	actSet->AddActivity(activity1);

	ActivityPtr activity2 = Activity::PrepareAdoptedActivity(2);
	actSet->AddActivity(activity2);

	Start(busClient, actSet);

	activity1->SetEndOrder(Activity::EndOrder_Last);

	End(busClient, actSet);

	// TODO check order ended
	ASSERT_TRUE(activity1->IsEnded());
	ASSERT_TRUE(activity2->IsEnded());
}
