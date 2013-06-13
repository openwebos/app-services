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

#ifndef EXCEPTIONUTILS_H_
#define EXCEPTIONUTILS_H_

#include <boost/exception_ptr.hpp>
#include "CommonErrors.h"

class ExceptionUtils
{
public:
	template<typename T>
	static bool CopyToExceptionPtr(const std::exception& e, boost::exception_ptr& ptr)
	{
		const T* specific = dynamic_cast<const T*>(&e);

		if(specific) {
			//fprintf(stderr, "exception is type %s\n", typeid(T).name());

			ptr = boost::copy_exception(*specific);
			return true;
		} else {
			//fprintf(stderr, "exception is not type %s\n", typeid(T).name());
			return false;
		}
	}

	static boost::exception_ptr CopyException(const std::exception& e);

	static std::string GetClassName(const std::exception& e);

	static void TerminateHandler();

	static MailError::ErrorInfo GetErrorInfo(const std::exception& e);

private:
	ExceptionUtils();
};

#endif /* EXCEPTIONUTILS_H_ */
