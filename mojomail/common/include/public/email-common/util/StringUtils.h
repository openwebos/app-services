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

#ifndef STRINGUTILS_H_
#define STRINGUTILS_H_

#include <string>

namespace StringUtils {
	extern const char* const UTF8_REPLACEMENT_CHAR;
	extern const char* const FILE_PREFIX;

	// Converts ASCII into valid UTF-8 by replacing non-ASCII chars with 0xFFFD
	void SanitizeASCII(std::string& text, const char* replacement = UTF8_REPLACEMENT_CHAR);

	std::string GetSanitizedASCII(const std::string& text, const char* replacement = UTF8_REPLACEMENT_CHAR);

	// Replaces invalid UTF-8 sequences
	void SanitizeUTF8(std::string& text);

	// Strips file:// off of local file paths
	void SanitizeFilePath(std::string& text);

	// Replace '/' and '\' to '_'
	void SanitizeFileCacheName(std::string& text);
};

#endif /* STRINGUTILS_H_ */
