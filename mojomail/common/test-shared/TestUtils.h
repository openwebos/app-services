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

#ifndef TESTUTILS_H_
#define TESTUTILS_H_

#include "core/MojObject.h"
#include "exceptions/MojErrException.h"

static inline MojObject StringAsMojObject(const char* str)
{
	MojErr err;
	MojString mojStr;
	err = mojStr.assign(str);
	MojErrException::CheckErr(err, __FILE__, __LINE__);

	MojObject obj;
	obj.assign(mojStr);

	return obj;
}

static inline MojObject MojObjectFromJson(const std::string& x, const char* filename, int line)
{
	MojObject obj;
	MojErr err = obj.fromJson(x.c_str());
	MojErrException::CheckErr(err, filename, line);
	return obj;
}

#include <boost/algorithm/string/trim.hpp>
#define QUOTE_JSON(x) (boost::trim_copy_if(std::string(#x), boost::is_any_of(" ()")))
#define QUOTE_JSON_OBJ(x) MojObjectFromJson(QUOTE_JSON(x), __FILE__, __LINE__)

#endif /* TESTUTILS_H_ */
