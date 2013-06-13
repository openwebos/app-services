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

#include "util/ObjectTracker.h"
#include <boost/foreach.hpp>
#include "exceptions/Backtrace.h"
#include <typeinfo>
#include "core/MojRefCount.h"

using namespace std;

set<const TrackedObject*> ObjectTracker::s_objects;

void ObjectTracker::Dump()
{
	DumpClass(NULL);
}

void ObjectTracker::DumpClass(const char* className)
{
	BOOST_FOREACH(const TrackedObject* obj, s_objects) {
		const std::type_info& info = typeid(*obj);

		// Need to use dynamic_cast to get the right pointer due to multiple inheritance
		const void* basePtr = dynamic_cast<const void*>(obj);

		string demangled = Backtrace::Demangle(info.name());

		if(className == NULL || demangled == className) {
			fprintf(stderr, "(%s*) %p\n", demangled.c_str(), basePtr);
		}
	}
}

template<typename T>
void ObjectTracker::Inspect(T* obj)
{
	const std::type_info& info = typeid(*obj);

	string demangled = Backtrace::Demangle(info.name());

	// Need to use dynamic_cast to get the right pointer due to multiple inheritance
	const void* ptr = dynamic_cast<const void*>(obj);

	fprintf(stderr, "(%s*) %p\n", demangled.c_str(), ptr);
}

// Useful debug function
void Inspect(MojRefCounted* ptr)
{
	ObjectTracker::Inspect<MojRefCounted>(ptr);
}
