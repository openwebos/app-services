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

#include "exceptions/Backtrace.h"
#include <cstdio>

using namespace std;

#ifdef __GNUC__
#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#endif

Backtrace::Backtrace(int maxDepth)
: addrs(NULL), count(0), maxDepth(maxDepth)
{
	if(maxDepth < 1) maxDepth = 1;
	addrs = new void*[maxDepth];

	GetBacktrace();
}

Backtrace::~Backtrace()
{
	delete [] addrs;
}

#ifdef __GNUC__

void Backtrace::GetBacktrace()
{
	count = backtrace(addrs, maxDepth);
}

#else

void Backtrace::GetBacktrace()
{
}

#endif

void Backtrace::PrintBacktrace()
{
	for(int i = 1; i < count; i++)
	{
		Dl_info info;
		if( dladdr(addrs[i], &info) ) {
			int offset = (char*) addrs[i] - (char*) info.dli_fbase;

			if(info.dli_sname) {
				string symbolName = Demangle(info.dli_sname);

				fprintf(stderr, "%p %s (%s+0x%x)\n", addrs[i], symbolName.c_str(), info.dli_fname, offset);
			} else {
				fprintf(stderr, "%p %s+0x%x\n", addrs[i], info.dli_fname, offset);
			}
		} else {
			fprintf(stderr, "%p\n", addrs[i]);
		}
	}
}

void Backtrace::AddTraceAddress(void *addr)
{
	if(count < maxDepth) addrs[count++] = addr;
}

std::string Backtrace::Demangle(const char* str)
{
	string output;

#ifdef __GNUC__
	if(str) {
		char buf[1024];
		size_t size = 1024;
		int status = 0;
		abi::__cxa_demangle(str, buf, &size, &status);
		if(status == 0) {
			output.assign(buf);
			return output;
		}
	}
#endif

	if (NULL != str)
		output.assign(str);

	return output;
}
