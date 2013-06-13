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

#include "util/Hexdump.h"
#include <sstream>
#include <iomanip>
#include <cstdio>

using namespace std;

std::string Hexdump::FormatBytes(const char* data, size_t length)
{
	stringstream ss;

	for(unsigned int i = 0; i < length; i++) {
		if(i > 0)
			ss << " ";
		ss << hex << setw(2) << setfill('0') << (unsigned int) (unsigned char) data[i];
	}

	return ss.str();
}

std::string Hexdump::FormatWrapped(const char* data, size_t length, unsigned int wrap)
{
	stringstream ss;

	for(unsigned int i = 0; i < length; i += wrap) {
		ss << FormatBytes(&data[i], std::min(wrap, length - i)) << "\n";
	}

	return ss.str();
}

void Hexdump::Dump(FILE* file, const char* fmt, const char* data, size_t length)
{
	if(file) {
		fprintf(file, fmt, FormatWrapped(data, length, 32).c_str());
	}
}
