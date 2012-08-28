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

#ifndef OBJECTTRACKER_H_
#define OBJECTTRACKER_H_

#include <set>

class TrackedObject;

class ObjectTracker
{
public:
	static void Dump();
	static void DumpClass(const char* className);

	template<typename T>
	static void Inspect(T* obj);

private:
	ObjectTracker();

	static inline void AddObject(const TrackedObject* ptr) { s_objects.insert(ptr); }
	static inline void RemoveObject(const TrackedObject* ptr) { s_objects.erase(ptr); }

	friend class TrackedObject;

	static std::set<const TrackedObject*> s_objects;
};

class TrackedObject
{
public:
	TrackedObject() { ObjectTracker::AddObject(this); }
	virtual ~TrackedObject() { ObjectTracker::RemoveObject(this); }
};

#endif /* OBJECTTRACKER_H_ */
