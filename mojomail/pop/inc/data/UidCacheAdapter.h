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

#ifndef UIDCACHEADAPTER_H_
#define UIDCACHEADAPTER_H_

#include <string>
#include "PopDefs.h"
#include "core/MojObject.h"
#include "data/DeletedEmailsCache.h"
#include "data/OldEmailsCache.h"
#include "data/UidCache.h"

class UidCacheAdapter
{
public:
	static const char* const 	EMAILS_UID_CACHE_KIND;

	static const char* const	KIND;
	static const char* const	ID;
	static const char* const	REV;
	static const char* const 	ACCOUNT_ID;
	static const char* const 	UID_CACHE_STRING;
	static const char* const 	DELETED_EMAILS_CACHE;
	static const char* const 	LOCAL_DELETED_EMAILS_CACHE;
	static const char* const 	PENDING_DELETED_EMAILS_CACHE;
	static const char* const	OLD_EMAILS_CACHE;

	/*
	 * Get data from MojoDB and turn them into UidCache
	 */
	static void		ParseDatabasePopObject(const MojObject& obj, UidCache& cache);
	/*
	 * Convert UidCache data into MojoDB.
	 */
	static void		SerializeToDatabasePopObject(UidCache& cache, MojObject& obj);
private:
	/**
	 * Converts the string representation of uid cache in MojoDB into UidCache object.
	 */
	static void	 	ParseUidCacheString(const MojString cacheStr, UidCache& cache);

	/**
	 * Converts the UidCache object into a string that will be persisted into
	 * MojoDB.
	 */
	static void		SerializeCacheMapToString(UidCache& cache, MojString& cacheStr);
};
#endif /* UIDCACHEADAPTER_H_ */
