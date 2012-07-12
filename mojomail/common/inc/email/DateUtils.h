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

/**
 * @file	DateUtils.h
 *
 * Copyright 2011 Palm, Inc.  All rights reserved.
 */
#ifndef DATEUTILS_H_
#define DATEUTILS_H_

#include <ctime>
#include "core/MojCoreDefs.h"
#include <string>


class DateUtils
{
public:
	// Currently a wrapper for curl_getdate(). Returns -1 on error.
	static time_t ParseRfc822Date(const char* str);

	static const MojInt64 GetCurrentTimeMillis();
	static const std::string GetUTC2DateString(MojInt64 val, const char* format);
	static const std::string GetLocalDateString(MojInt64 val, const char* format);
	static const std::string GetLocalDateStringFromUTC(MojInt64 utcVal, const char* format);

private:
	DateUtils();
};

#endif /* DATEUTILS_H_ */
