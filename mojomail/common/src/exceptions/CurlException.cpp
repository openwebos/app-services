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

#include "curl/curl.h"
#include "exceptions/CurlException.h"

using namespace std;

CurlException::CurlException(const CURLcode code, const char* file, int line)
: MailException(curl_easy_strerror(code), file, line),
  m_curlCode(code),
  m_curlmCode(CURLM_OK)
{
	stringstream msg;
	msg << curl_easy_strerror(m_curlCode) << " (CURLcode " << m_curlCode << ")";
	SetMessage(msg.str().c_str());
}

CurlException::CurlException(const CURLMcode mCode, const char* file, int line)
: MailException(curl_multi_strerror(mCode), file, line),
  m_curlCode(CURLE_OK),
  m_curlmCode(mCode)
{
	stringstream msg;
	msg << curl_multi_strerror(m_curlmCode) << " (CURLMcode " << m_curlmCode << ")";
	SetMessage(msg.str().c_str());
}
