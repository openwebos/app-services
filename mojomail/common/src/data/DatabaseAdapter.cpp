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

#include "data/DatabaseAdapter.h"
#include "db/MojDbQuery.h"
#include "CommonPrivate.h"

const char* const DatabaseAdapter::ID = "_id";
const char* const DatabaseAdapter::KIND = "_kind";
const char* const DatabaseAdapter::REV = "_rev";
const char* const DatabaseAdapter::DEL = "_del";

const char* const DatabaseAdapter::RESULTS = "results";

// These are for put/merge results
const char* const DatabaseAdapter::RESULT_ID = "id";
const char* const DatabaseAdapter::RESULT_REV = "rev";

// Count
const char* const DatabaseAdapter::COUNT = "count";

const int DatabaseAdapter::RESERVE_BATCH_SIZE = 500;

using namespace std;

void DatabaseAdapter::GetOneResult(const MojObject& response, MojObject& obj)
{
	MojErr err;
	MojObject results;

	err = response.getRequired("results", results);
	ErrorToException(err);

	if(results.size() < 1) {
		throw MailException("no results found", __FILE__, __LINE__);
	}

	if(results.size() > 1) {
		throw MailException("too many results", __FILE__, __LINE__);
	}

	if(!results.at(0, obj)) {
		throw MailException("error getting result", __FILE__, __LINE__);
	}
}

std::pair<MojObject::ConstArrayIterator, MojObject::ConstArrayIterator> DatabaseAdapter::GetArrayIterators(const MojObject& array)
{
	if(array.type() == MojObject::TypeArray) {
		return std::make_pair<MojObject::ConstArrayIterator, MojObject::ConstArrayIterator>(array.arrayBegin(), array.arrayEnd());
	} else {
		throw MailException("not an array", __FILE__, __LINE__);
	}
}

std::pair<MojObject::ConstArrayIterator, MojObject::ConstArrayIterator> DatabaseAdapter::GetResultsIterators(const MojObject& response)
{
	MojObject results;
	MojErr err = response.getRequired("results", results);
	ErrorToException(err);

	return GetArrayIterators(results);
}

bool DatabaseAdapter::GetNextPage(const MojObject& response, MojDbQuery::Page& page)
{
	MojErr err;
	MojObject next;

	page.clear();

	if(response.contains("next")) {
		err = response.getRequired("next", next);
		ErrorToException(err);

		err = page.fromObject(next);
		ErrorToException(err);

		return true;
	}

	return false;
}

bool DatabaseAdapter::GetOptionalBool(const MojObject& object, const char* field, bool defaultValue)
{
	if(object.contains(field)) {
		bool value = false;
		MojErr err = object.getRequired(field, value);
		ErrorToException(err);
		return value;
	}
	return defaultValue;
}

std::string DatabaseAdapter::GetOptionalString(const MojObject& object, const char* field)
{
	bool hasField = false;
	MojString value;
	MojErr err = object.get(field, value, hasField);
	ErrorToException(err);

	if(hasField) {
		return std::string(value.data());
	}
	return "";
}

void DatabaseAdapter::PutOptionalString(MojObject& object, const char* field, const string& str)
{
	if(!str.empty()) {
		MojErr err = object.putString(field, str.c_str());
		ErrorToException(err);
	}
}
