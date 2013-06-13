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

#include "ImapConfig.h"
#include "core/MojObject.h"
#include "ImapPrivate.h"
#include "data/DatabaseAdapter.h"

const int ImapConfig::DEFAULT_INACTIVITY_TIMEOUT = 30; // 30 minutes
const int ImapConfig::DEFAULT_HEADER_BATCH_SIZE = 20; // 20 headers at a time
const int ImapConfig::DEFAULT_MAX_EMAILS = 1000; // 1000 emails per folder max
const int ImapConfig::DEFAULT_CONNECT_TIMEOUT = 20; // 20 seconds
const int ImapConfig::DEFAULT_SESSION_KEEPALIVE = 50; // 50 seconds
const int ImapConfig::DEFAULT_NUM_AUTODOWNLOAD_BODIES = 1000; // 1000 email bodies

ImapConfig ImapConfig::s_instance;

ImapConfig::ImapConfig()
: m_inactivityTimeout(DEFAULT_INACTIVITY_TIMEOUT),
  m_headerBatchSize(DEFAULT_HEADER_BATCH_SIZE),
  m_maxEmails(DEFAULT_MAX_EMAILS),
  m_ignoreViewFolder(false),
  m_ignoreNetworkStatus(false),
  m_connectTimeout(DEFAULT_CONNECT_TIMEOUT),
  m_cleanDisconnect(false),
  m_enableCompress(true),
  m_sessionKeepAlive(DEFAULT_SESSION_KEEPALIVE),
  m_keepAliveForSync(false),
  m_numAutoDownloadBodies(DEFAULT_NUM_AUTODOWNLOAD_BODIES)
{
}

ImapConfig::~ImapConfig()
{
}

void ImapConfig::GetOptionalInt(const MojObject& obj, const char* prop, int& value, int minValue, int maxValue)
{
	bool hasProp = false;
	int temp = value;

	MojErr err = obj.get(prop, temp, hasProp);
	ErrorToException(err);

	if(hasProp) {
		value = std::max(minValue, std::min(temp, maxValue));
	}
}

void ImapConfig::GetOptionalBool(const MojObject& obj, const char* prop, bool& value)
{
	bool hasProp = false;
	int temp = value;

	MojErr err = obj.get(prop, temp, hasProp);
	ErrorToException(err);

	if(hasProp) {
		value = temp;
	}
}

MojErr ImapConfig::ParseConfig(const MojObject& conf)
{
	GetOptionalInt(conf, "inactivityTimeoutSeconds", m_inactivityTimeout, 0, MojInt32Max);
	GetOptionalInt(conf, "headerBatchSize", m_headerBatchSize, 1, 1000);
	GetOptionalInt(conf, "maxEmails", m_maxEmails, 100, 20000);
	GetOptionalBool(conf, "ignoreViewFolder", m_ignoreViewFolder);
	GetOptionalBool(conf, "ignoreNetworkStatus", m_ignoreNetworkStatus);
	GetOptionalInt(conf, "connectTimeout", m_connectTimeout, 0, 120);
	GetOptionalInt(conf, "sessionKeepAlive", m_sessionKeepAlive, 0, 5 * 60);
	GetOptionalBool(conf, "keepAliveForSync", m_keepAliveForSync);
	GetOptionalBool(conf, "cleanDisconnect", m_cleanDisconnect);
	GetOptionalBool(conf, "enableCompress", m_enableCompress);

	return MojErrNone;
}
