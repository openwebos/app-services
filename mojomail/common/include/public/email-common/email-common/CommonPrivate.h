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

#ifndef COMMONPRIVATE_H_
#define COMMONPRIVATE_H_

// Includes useful header definitions. Should ONLY be included from .cpp files, not other headers.

#include "CommonDefs.h"
#include "exceptions/MojErrException.h"
#include "boost/make_shared.hpp"
#include "core/MojObject.h"
#include <string>
#include "util/LogUtils.h"
#include "CommonMacros.h"

#define GErrorToException(err) \
	do { \
		if(unlikely(err != NULL)) GErrorException::CheckError(err, __FILE__, __LINE__); \
	} while(0);

inline bool IsValidId(const MojObject& obj) { return !obj.undefined() && !obj.null(); }

#endif /*COMMONPRIVATE_H_*/
