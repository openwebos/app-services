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

#ifndef IMAPPRIVATE_H_
#define IMAPPRIVATE_H_

// Misc useful imports and definitions. Should only be used in .cpp files

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include <string>
#include <core/MojObject.h>
#include "ImapCoreDefs.h"
#include <boost/exception_ptr.hpp>

using namespace std;
using boost::make_shared;

#define UNKNOWN_EXCEPTION boost::unknown_exception()

inline bool IsValidId(const MojObject& obj) { return !obj.undefined() && !obj.null(); }

inline string AsJsonString(const MojObject& obj)
{
	MojString str;
	MojErr err = obj.toJson(str);
	ErrorToException(err);
	return string(str.data());
}

#endif /*IMAPPRIVATE_H_*/
