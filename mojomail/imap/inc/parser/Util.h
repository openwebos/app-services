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

/*
 * @file	Util.h
 */


#ifndef UTIL_H_
#define UTIL_H_

#include <sstream>
#include <iostream>
#include "core/MojCoreDefs.h"

class Util {
public:

	static std::wstring encodeModifiedUtf7(const std::wstring& str);

	static std::string decodeModifiedUtf7(const std::string& str);

	static std::wstring quoteString(const std::wstring& str);

	static std::string encodeFolderName(const std::string& folderName);

	static std::string toString(int n);

	static std::string toHexString(int n, int width=2);

	static inline std::string trimmed( std::string const& str, char const* sepSet = "\r\n")
	{
		std::string::size_type const first = str.find_first_not_of(sepSet);
		return ( first==std::string::npos )? std::string() : str.substr(first, str.find_last_not_of(sepSet)-first+1);
	}

	template <class T>
	static inline bool from_string(T& t,
	                 const std::string& s,
	                 std::ios_base& (*f)(std::ios_base&))
	{
	  std::istringstream iss(s);
	  return !(iss >> f >> t).fail();
	}
	
	static MojInt64 ParseInternalDate(const std::string& s);
};

#endif /* UTIL_H_ */
