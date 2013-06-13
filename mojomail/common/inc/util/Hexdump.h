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

#ifndef HEXDUMP_H_
#define HEXDUMP_H_

#include <string>

namespace Hexdump {
	std::string FormatBytes(const char* data, size_t length);
	std::string FormatWrapped(const char* data, size_t length, unsigned int wrap);

	void Dump(FILE* file, const char* fmt, const char* data, size_t length);
};

#endif /* HEXDUMP_H_ */
