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

#include "client/Capabilities.h"
#include <boost/algorithm/string/case_conv.hpp>

using namespace std;

const string Capabilities::IDLE				= "IDLE";
const string Capabilities::XLIST			= "XLIST";
const string Capabilities::UIDPLUS			= "UIDPLUS";
const string Capabilities::NAMESPACE		= "NAMESPACE";
const string Capabilities::STARTTLS			= "STARTTLS";
const string Capabilities::LOGINDISABLED	= "LOGINDISABLED";
const string Capabilities::COMPRESS_DEFLATE	= "COMPRESS=DEFLATE";

Capabilities::Capabilities()
: m_valid(false)
{
}

Capabilities::~Capabilities()
{
}

void Capabilities::SetCapability(const string& s)
{
	// FIXME: force uppercase?
	m_caps.insert(s);
}

void Capabilities::RemoveCapability(const string& s)
{
	m_caps.erase(s);
}

void Capabilities::Clear()
{
	m_valid = false;
	m_caps.clear();
}
