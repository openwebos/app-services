// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "data/PopAccount.h"

const char* const PopAccount::SOCKET_ENCRYPTION_NONE	= "none";
const char* const PopAccount::SOCKET_ENCRYPTION_SSL		= "ssl";
const char* const PopAccount::SOCKET_ENCRYPTION_TLS		= "tls";
const char* const PopAccount::SOCKET_ENCRYPTION_TLS_IF_AVAILABLE		= "tlsIfAvailable";
const int PopAccount::DEFAULT_SYNC_WINDOW				= 7;   // sync 7 days back
const int PopAccount::DEFAULT_SYNC_FREQUENCY_MINS		= 30;  // default is to sync every 30 min

PopAccount::PopAccount()
: EmailAccount(),
  m_port(0),
  m_initialSync(true),
  m_deleteFromServer(true),
  m_deleteOnDevice(false)
{
	m_error.errorCode = MailError::VALIDATED;
}

PopAccount::~PopAccount()
{

}
