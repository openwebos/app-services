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

#include "sync/UIDMap.h"
#include <gtest/gtest.h>

using namespace std;

TEST(UIDMapTest, TestLimit)
{
	UID arr[] = {3, 5, 8, 13, 21, 34, 55, 89, 144, 233, 377, 610, 987, 1597, 2584, 4181, 6765, 10946, 17711, 28657};

	vector<UID> uids;
	uids.insert(uids.begin(), &arr[4], &arr[20]); // listed message numbers: 5 to 20

	UIDMap uidMap(uids, 20, 10);

	EXPECT_EQ( UID(28657), uidMap.GetUID(20) );
	EXPECT_EQ( UID(377), uidMap.GetUID(11) );
	EXPECT_EQ( UID(610), uidMap.GetUID(12) );
	EXPECT_EQ( UID(0), uidMap.GetUID(10) ); // not in map
}
