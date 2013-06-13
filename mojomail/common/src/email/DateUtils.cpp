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

/**
 * @file	DateUtils.cpp
 */
#include "email/DateUtils.h"
#include <curl/curl.h>


time_t DateUtils::ParseRfc822Date(const char* str)
{
	return curl_getdate(str, NULL);
}

const MojInt64 DateUtils::GetCurrentTimeMillis()
{
	time_t nowSec = time(NULL);
	return MojInt64(nowSec) * 1000;
}

const std::string DateUtils::GetUTC2DateString(MojInt64 val, const char* format)
{
	char buffer [50];
	struct tm *newtime;
	time_t longtime = val/1000;
	newtime = gmtime(&longtime);
	strftime(buffer,50,format,newtime);
	return buffer;
}

const std::string DateUtils::GetLocalDateString(MojInt64 val, const char* format)
{
	char buffer [50];
	struct tm *newtime;
	time_t longtime = val/1000;
	newtime = localtime(&longtime);
	strftime(buffer,50,format,newtime);
	return buffer;
}

const std::string DateUtils::GetLocalDateStringFromUTC(MojInt64 utcVal, const char* format)
{
	char buffer [50];
	struct tm newLocalTime;
	time_t longGmTime = utcVal/1000;
	localtime_r(&longGmTime, &newLocalTime);
	strftime(buffer,50,format, &newLocalTime);
	return buffer;
}
