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

#ifndef DATABASEADAPTER_H_
#define DATABASEADAPTER_H_

#include "core/MojObject.h"
#include "db/MojDbQuery.h"
#include <utility>
#include <string>

class DatabaseAdapter {
public:
	static const char* const ID;
	static const char* const KIND;
	static const char* const REV;
	static const char* const DEL;

	static const char* const RESULTS;

	static const char* const RESULT_ID;
	static const char* const RESULT_REV;

	static const char* const COUNT;

	static const int RESERVE_BATCH_SIZE;

	static void GetOneResult(const MojObject& response, MojObject& obj);

	// Returns a pair of iterators over the database results.
	// Can be used like: BOOST_FOREACH(const MojObject& obj, DatabaseAdapter::GetResultsIterators(response))
	static std::pair<MojObject::ConstArrayIterator, MojObject::ConstArrayIterator> GetResultsIterators(const MojObject& response);

	// Get iterator over a MojArray
	static std::pair<MojObject::ConstArrayIterator, MojObject::ConstArrayIterator> GetArrayIterators(const MojObject& response);

	// Get the next page key
	static bool GetNextPage(const MojObject& response, MojDbQuery::Page& page);

	// Get an optional boolean field
	static bool GetOptionalBool(const MojObject& object, const char* field, bool defaultValue = false);

	// Get an optional string. Returns an empty string if the field does not exist.
	static std::string GetOptionalString(const MojObject& object, const char* field);

	static void PutOptionalString(MojObject& object, const char* field, const std::string& str);

private:
	DatabaseAdapter();
};

#endif /* DATABASEADAPTER_H_ */
