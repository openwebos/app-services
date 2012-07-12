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

#include "email/CharsetSupport.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/erase.hpp>

using namespace std;

const char* CharsetSupport::SelectCharset(const char* charset)
{
	string normalized = charset;
	boost::to_lower(normalized);
	boost::erase_all(normalized, "-");
	boost::erase_all(normalized, "_");

	// Often times, text with these charsets actually contain GB18030 characters.
	// GB18030 is a superset of these, so it's safe to decode using it instead.
	if(normalized == "gb2312" || normalized == "gbk" || normalized == "xgbk") {
		return "gb18030";
	}

	return charset;
}
