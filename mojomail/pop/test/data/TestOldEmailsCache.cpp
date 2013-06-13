// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "data/OldEmailsCache.h"
#include "core/MojObject.h"
#include <gtest/gtest.h>
#include <boost/lexical_cast.hpp>

using namespace std;

string UID_PREFIX = "uid";

typedef std::vector<MojInt64> CacheValues;

void RegularInsert(OldEmailsCache& cache, CacheValues& values)
{
	long limit = 200;

	for (long ndx = 0; ndx < limit; ndx++) {
		MojInt64 timestamp = ndx;
		std::string uid = UID_PREFIX;
		uid += boost::lexical_cast<string>(ndx);

		// add uid-timestamp pair to cache
		cache.AddToCache(uid, timestamp);
		values.push_back(ndx);
	}
}

void ExcessInsert(OldEmailsCache& cache, CacheValues& values)
{
	long limit = OldEmailsCache::CACHE_SIZE_LIMIT + 100;

	for (long ndx = 0; ndx < limit; ndx++) {
		MojInt64 timestamp = ndx;
		std::string uid = UID_PREFIX;
		uid += boost::lexical_cast<string>(ndx);

		// add uid-timestamp pair to cache
		cache.AddToCache(uid, timestamp);
		values.push_back(ndx);
	}

	// the first 100 elements should be pushed out of the cache, so adjust the
	// cache value list so that the first 100 elements are removed.
	CacheValues::iterator begin = values.begin();

	values.erase(begin, begin + (limit - OldEmailsCache::CACHE_SIZE_LIMIT));
}

void ReverseExcessInsert(OldEmailsCache& cache, CacheValues& values)
{
	int limit = OldEmailsCache::CACHE_SIZE_LIMIT + 100;
	int cacheLimit = OldEmailsCache::CACHE_SIZE_LIMIT;

	for (long ndx = limit - 1; ndx >= 0; ndx--) {
		MojInt64 timestamp = ndx;
		std::string uid = UID_PREFIX;
		uid += boost::lexical_cast<string>(ndx);

		// add uid-timestamp pair to cache
		cache.AddToCache(uid, timestamp);
		values.push_back(timestamp);
	}

	// the last 100 elements should be pushed out of the cache, so adjust the
	// cache value list so that the last 100 elements are removed.
	CacheValues::iterator begin = values.begin();
	values.erase(begin + cacheLimit, values.end());
}

void VerifyLookup(OldEmailsCache& cache, CacheValues& values)
{
	long ndx = 0;
	for (CacheValues::iterator itr = values.begin(); itr < values.end(); itr++, ndx++) {
		MojInt64 timestamp = *itr;
		std::string uid = UID_PREFIX;
		uid += boost::lexical_cast<string>(timestamp);

		MojInt64 cachedTs = cache.GetEmailTimestamp(uid);

		if (cachedTs != timestamp) {
			fprintf(stderr, "Expecting %lld for UID=%s, but found %lld\n", timestamp, uid.c_str(), cachedTs);
			FAIL() << ("Cache value not found");
		}
	}

}

TEST(TestOldEmailsCache, TestCacheRegular)
{
	OldEmailsCache cache;
	CacheValues values;

	RegularInsert(cache, values);
	VerifyLookup(cache, values);
}

TEST(TestOldEmailsCache, TestCacheExcess)
{
	OldEmailsCache cache;
	CacheValues values;

	ExcessInsert(cache, values);
	VerifyLookup(cache, values);
}

TEST(TestOldEmailsCache, TestCacheReverseExcess)
{
	OldEmailsCache cache;
	CacheValues values;

	ReverseExcessInsert(cache, values);
	VerifyLookup(cache, values);
}
