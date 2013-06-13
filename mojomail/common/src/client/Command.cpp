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

#include "client/Command.h"
#include "core/MojObject.h"
#include "CommonPrivate.h"
#include <typeinfo>

#ifdef __GNUC__
#include <cxxabi.h>
#endif

std::string Command::GetClassName() const
{
	const std::type_info& info = typeid(*this);
	std::string className;

#ifdef __GNUC__
	char buf[1024];
	size_t size = 1024;
	int status = 0;
	abi::__cxa_demangle(info.name(), buf, &size, &status);
	if(status == 0) {
		className.assign(buf);
	} else {
		className.assign(info.name());
	}
#else
	className = info.name();
#endif
	return className;
}

void Command::Status(MojObject& status) const
{
	MojErr err;

	err = status.putString("class", GetClassName().c_str());
	ErrorToException(err);
	err = status.putInt("priority", m_priority);
	ErrorToException(err);
}
