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

#include "email/PreviewTextGenerator.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/find_format.hpp>
#include <boost/algorithm/string/regex_find_format.hpp>
#include <sstream>
#include <glib.h>
#include <exception>
#include "email/HtmlEntities.h"

using namespace std;

#define BLOCK_TAG(x) "<[[:space:]]*" x "[^>]*>.*?<[[:space:]]*/" x "[[:space:]]*>"

boost::regex headerTagRegex(BLOCK_TAG("style") "|" BLOCK_TAG("head") "|" BLOCK_TAG("script"), boost::regex::icase);
boost::regex htmlTagRegex("<[^<][^<>]*(>|$)|&nbsp;", boost::regex::icase);
boost::regex whitespaceRegex("\\s+");
boost::regex entityRegex("&[^;]+;");

string PreviewTextGenerator::GeneratePreviewText(const string& text, size_t maxLength, bool html)
{
	string temp = text;

	if(html) {
		boost::smatch m;

		// strip header, script, and style tags
		temp = boost::regex_replace(temp, headerTagRegex, "");

		// strip html tags
		temp = boost::regex_replace(temp, htmlTagRegex, " ");

		// Fixup HTML entities
		// Note that unescaping must be done in one pass to avoid double-unescaping,
		// e.g. '&amp;gt;' should appear as '&gt;' not '<'
		temp = ReplaceEntities(temp);
	}

	// cleanup whitespace
	temp = boost::regex_replace(temp, whitespaceRegex, " ");

	boost::trim(temp);
	TruncateUTF8(temp, maxLength);

	return temp;
}

class EntityFormatter
{
public:
	template<typename IteratorT>
	string operator()(const boost::iterator_range<IteratorT>& range) const
	{
		gunichar charValue = 0;

		try {
			// Numeric value entities
			if(range.size() > 3 && range[1] == '#') {
				if(range[2] == 'x' || range[2] == 'X') { // hex, e.g. &#x00A2;
					istringstream ss( string(range.begin() + 3, range.end() - 1) );
					ss >> std::hex >> charValue;

					// Not valid if it couldn't read the whole string
					if(ss.fail() || !ss.eof()) {
						charValue = 0;
					}

				} else { // decimal, e.g. &#1234
					istringstream ss( string(range.begin() + 2, range.end() - 1) );
					ss >> std::dec >> charValue;

					// Not valid if it couldn't read the whole string
					if(ss.fail() || !ss.eof()) {
						charValue = 0;
					}
				}

				// Check if it's a valid Unicode value
				if(charValue <= 0 || !g_unichar_validate(charValue)) {
					charValue = 0;
				}
			} else if(range.size() > 2) {
				string entityName = string(range.begin() + 1, range.end() - 1);

				charValue = HtmlEntities::LookupEntity(entityName);
			}
		} catch(...) {
			// ignore exceptions
		}

		// Make sure it's a valid value
		if(charValue > 0) {
			// Convert to UTF-8; buffer size must be at least 6 bytes
			char buf[6];
			gint length = g_unichar_to_utf8(charValue, buf);

			return string(buf, length);
		} else {
			// Couldn't convert into a Unicode char; return the original string
			return string(range.begin(), range.end());
		}
	}
};

string PreviewTextGenerator::ReplaceEntities(const string& text)
{
	return boost::find_format_all_copy(text, boost::regex_finder(entityRegex), EntityFormatter());
}

void PreviewTextGenerator::TruncateUTF8(string& text, size_t maxLength) {

	if(text.length() > maxLength) {
		// Find the start of the last UTF-8 character
		gchar* end = g_utf8_find_prev_char(text.data(), text.data() + maxLength);

		if(end != NULL && end >= text.data()) {
			// Chop off the last UTF-8 character, in case it's incomplete
			text.resize(end - text.data());
		} else {
			// Couldn't find the start of the last UTF-8 character.
			// Strip out high-ASCII characters from the end.
			text.resize( std::min(text.size(), maxLength) );
			boost::trim_right_if(text, !boost::is_from_range(0, 127));
		}
	}
}
