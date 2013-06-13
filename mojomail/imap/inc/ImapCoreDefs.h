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

#ifndef IMAPCOREDEFS_H_
#define IMAPCOREDEFS_H_

#include "exceptions/MojErrException.h"
#include "ImapStatus.h"
#include <boost/shared_ptr.hpp>
#include "CommonDefs.h"

typedef MojUInt32 UID;

class ImapEmail;
class ImapFolder;
typedef boost::shared_ptr<ImapFolder> ImapFolderPtr;

#define ErrorToException(err) \
	do { \
		if(unlikely(err)) MojErrException::CheckErr(err, __FILE__, __LINE__); \
	} while(0);

#define ResponseToException(response, err) \
	do { \
		if(unlikely(err)) MojErrException::CheckErr(response, err, __FILE__, __LINE__); \
	} while(0);

#endif /*IMAPCOREDEFS_H_*/
