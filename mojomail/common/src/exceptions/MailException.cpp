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

#include "exceptions/MailException.h"

#include <sstream>

using namespace std;

MailException::MailException(const char* file, int line)
: m_file(file),
  m_line(line)
{

}

MailException::MailException(const char* msg, const char* file, int line)
: m_file(file),
  m_line(line)
{
	SetMessage(msg);
}

void MailException::SetMessage(const char* msg)
{
	m_msg.assign(msg);

	// convert line to string
	stringstream ss;
	ss << m_line;

	m_msg.append(" -- ").append(m_file).append(":").append(ss.str());
}

const char* MailException::what() const throw()
{
	return m_msg.c_str();
}

MailError::ErrorInfo MailException::GetErrorInfo() const
{
	return MailError::ErrorInfo(MailError::INTERNAL_ERROR, what());
}
