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

#include "util/StringUtils.h"
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "exceptions/MailException.h"
#include <glib.h>

using namespace std;

const char* const StringUtils::UTF8_REPLACEMENT_CHAR 	= "\xef\xbf\xbd";
const char* const StringUtils::FILE_PREFIX 				= "file://";

void StringUtils::SanitizeASCII(string& text, const char* replacement)
{
	// Replace all unprintable characters
	boost::find_format_all(text, boost::token_finder(!(boost::is_print() || boost::is_space()), boost::token_compress_on), boost::const_formatter(replacement));
}

string StringUtils::GetSanitizedASCII(const string& text, const char* replacement)
{
	string out = text;
	StringUtils::SanitizeASCII(out, replacement);
	return out;
}

void StringUtils::SanitizeUTF8(string& text)
{
	const char* start = text.data();
	const char* end = text.data() + text.length();
	const gchar* endValid = NULL;

	string temp;

	do {
		bool valid = g_utf8_validate(start, end - start, &endValid);

		if(valid && temp.empty()) {
			// string is good as-is
			return;
		}

		// Copy everything up to this point
		temp.append(start, endValid);

		if(!valid) {
			// Add the replacement char
			temp.append(UTF8_REPLACEMENT_CHAR);
		}

		// Find the next valid char
		const char* newStart = g_utf8_find_next_char(endValid, end);
		
		// Make sure we can't get stuck in a loop; this shouldn't happen though
		if(newStart != NULL && newStart <= start) {
			throw MailException("error sanitizing", __FILE__, __LINE__);
		}
		
		start = newStart;
	} while(start != NULL && start < end);

	// Replace text
	text.assign(temp);
}

void StringUtils::SanitizeFilePath(string& text)
{
	const int len 		= text.length();
	const int prefixLen	= strlen(FILE_PREFIX);

	// Strip out file://
	if (boost::starts_with(text, FILE_PREFIX) && len > prefixLen) {
		text = text.substr(prefixLen, len-prefixLen);
	}
}

void StringUtils::SanitizeFileCacheName(string& text)
{
	// Replace all '/' and '\'
	boost::find_format_all(text, boost::first_finder("/", boost::is_iequal()), boost::const_formatter("_"));
	boost::find_format_all(text, boost::first_finder("\\", boost::is_iequal()), boost::const_formatter("_"));
}
