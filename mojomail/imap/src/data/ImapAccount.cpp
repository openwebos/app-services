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

#include "data/ImapAccount.h"

using namespace std;

ImapAccount::ImapAccount()
: m_compress(false),
  m_expunge(true),
  m_hasPassword(false)
{
}

ImapAccount::~ImapAccount()
{
}

const string& ImapAccount::GetTemplateId() const
{
	return m_templateId;
}

void ImapAccount::SetTemplateId(const char* templateId)
{
	m_templateId = templateId;
}

bool ImapAccount::IsYahoo()
{
	return m_templateId == "com.palm.yahoo";
}

bool ImapAccount::IsGoogle()
{
	return m_templateId == "com.palm.google";
}
