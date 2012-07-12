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

#ifndef TESTOLDEMAILSCACHE_H_
#define TESTOLDEMAILSCACHE_H_

#include <vector>
#include "cxxtest/TestSuite.h"
#include "core/MojObject.h"

class TestOldEmailsCache : public CxxTest::TestSuite
{
public:
	TestOldEmailsCache();
	virtual ~TestOldEmailsCache();

	void TestCacheRegular();
	void TestCacheExcess();
	void TestCacheReverseExcess();
private:
	static const char* const 	UID_PREFIX;

	typedef std::vector<MojInt64> CacheValues;

	void RegularInsert(OldEmailsCache& cache, CacheValues& values);
	void ExcessInsert(OldEmailsCache& cache, CacheValues& values);
	void ReverseExcessInsert(OldEmailsCache& cache, CacheValues& values);

	void VerifyLookup(OldEmailsCache& cache, CacheValues& values);
};

#endif /* TESTOLDEMAILSCACHE_H_ */
