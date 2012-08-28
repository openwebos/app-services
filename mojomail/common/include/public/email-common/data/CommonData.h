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

#ifndef COMMONDDATA_H_
#define COMMONDDATA_H_

#include "CommonDefs.h"
#include <vector>

// Email.h
class	Email;
typedef boost::shared_ptr<Email> EmailPtr;

// EmailAddress.h
class	EmailAddress;
typedef	boost::shared_ptr<EmailAddress> EmailAddressPtr;
typedef	std::vector<EmailAddressPtr> EmailAddressList;
typedef	boost::shared_ptr<EmailAddressList> EmailAddressListPtr;

// EmailPart.h
class	EmailPart;
typedef	boost::shared_ptr<EmailPart> EmailPartPtr;
typedef	std::vector<EmailPartPtr> EmailPartList;

#endif /*COMMONDATA_H_*/
