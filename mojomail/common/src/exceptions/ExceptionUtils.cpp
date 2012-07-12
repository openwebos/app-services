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

#include "exceptions/ExceptionUtils.h"
#include "exceptions/MailException.h"
#include "exceptions/GErrorException.h"
#include "core/MojCoreDefs.h"
#include "core/MojLogEngine.h"
#include "exceptions/Backtrace.h"
#include <typeinfo>

using namespace std;

boost::exception_ptr ExceptionUtils::CopyException(const std::exception& e)
{
	boost::exception_ptr ep;

	if(CopyToExceptionPtr<MailException>(e, ep)) {
		if(CopyToExceptionPtr<PslErrorException>(e, ep)) return ep;
		if(CopyToExceptionPtr<GErrorException>(e, ep)) return ep;
		if(CopyToExceptionPtr<MailNetworkDisconnectionException>(e, ep)) return ep;
		if(CopyToExceptionPtr<MailNetworkTimeoutException>(e, ep)) return ep;
		if(CopyToExceptionPtr<MailFileCacheNotCreatedException>(e, ep)) return ep;

		return ep;
	}

	// copy as std::exception
	return boost::copy_exception(e);
}

string ExceptionUtils::GetClassName(const exception& e)
{
	string name;

	const type_info& exceptionType = typeid(e);
	return Backtrace::Demangle(exceptionType.name());
}

void ExceptionUtils::TerminateHandler()
{
	static bool inHandler = false;

	if(inHandler) {
		// uh oh
		abort();
	}

	inHandler = true;

	string name = "unknown";
	string desc;

	try {
		if(boost::current_exception()) {
			try {
				throw;
			} catch(const exception& e) {

				name = GetClassName(e);
				desc = e.what();
			} catch(...) {
				// will use "unknown"
			}
		}

		MojLogger* logger = MojLogEngine::instance()->defaultLogger();

		if(logger) {
			if(!desc.empty())
				MojLogCritical(*logger, "terminate() called after throwing %s: %s", name.c_str(), desc.c_str());
			else
				MojLogCritical(*logger, "terminate() called after throwing %s", name.c_str());
		}

#ifndef __GNUC__
		if(!desc.empty())
			fprintf(stderr, "terminate() called after throwing %s: %s", name.c_str(), desc.c_str());
		else
			fprintf(stderr, "terminate() called after throwing %s", name.c_str());
#endif

	} catch(...) {
		// whoops!
	}

#ifdef __GNUC__
	__gnu_cxx::__verbose_terminate_handler();
#else
	// Must terminate
	abort();
#endif
}

MailError::ErrorInfo ExceptionUtils::GetErrorInfo(const std::exception& e)
{
	const MailException* mailException = dynamic_cast<const MailException*>(&e);
	if(mailException) {
		return mailException->GetErrorInfo();
	}

	return MailError::ErrorInfo(MailError::INTERNAL_ERROR, e.what());
}
