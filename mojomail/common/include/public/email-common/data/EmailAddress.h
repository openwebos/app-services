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

#ifndef EMAILADDRESS_H_
#define EMAILADDRESS_H_

#include <string>
#include "CommonData.h"

class EmailAddress
{
public:
	EmailAddress(const std::string& displayName, const std::string& address);
	
	EmailAddress(const std::string& address);
	
	virtual ~EmailAddress();
	
	const std::string&	GetAddress() const		{ return m_address; }
	const std::string&	GetDisplayName() const	{ return m_displayName; }
	
	bool HasDisplayName() const { return !m_displayName.empty(); }
	
protected:
	std::string m_displayName; // Should be UTF-8 encoded
	std::string m_address;
};

#endif /*EMAILADDRESS_H_*/
