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

#include "exceptions/MailErrorException.h"
#include "CommonErrors.h"
#include <sstream>

using namespace std;

MailErrorException::MailErrorException(MailError::ErrorInfo errorInfo, const char* filename, int line)
: MailException(filename, line),
  m_errorInfo(errorInfo)
{
	stringstream ss;

	if(!errorInfo.errorText.empty())
		ss << "MailError " << errorInfo.errorCode << ": " << errorInfo.errorText;
	else
		ss << "MailError " << (int) errorInfo.errorCode;

	SetMessage(ss.str().c_str());
}

MailError::ErrorInfo MailErrorException::GetErrorInfo() const
{
	return m_errorInfo;
}
