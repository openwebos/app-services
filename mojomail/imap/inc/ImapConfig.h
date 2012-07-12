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

#ifndef IMAPCONFIG_H_
#define IMAPCONFIG_H_

#include "core/MojCoreDefs.h"
#include <string>

/**
 * Miscellaneous runtime-configurable variables
 */
class ImapConfig
{
public:
	ImapConfig();
	virtual ~ImapConfig();

	// Parse config
	MojErr ParseConfig(const MojObject& config);

	// Get global instance
	static ImapConfig& GetConfig() { return s_instance; }

	int GetInactivityTimeout() const { return m_inactivityTimeout; }
	void SetInactivityTimeout(int timeout) { m_inactivityTimeout = timeout; }

	int GetHeaderBatchSize() const { return m_headerBatchSize; }
	void SetHeaderBatchSize(int size) { m_headerBatchSize = size; }

	int GetMaxEmails() const { return m_maxEmails; }
	void SetMaxEmails(int maxEmails) { m_maxEmails = maxEmails; }

	bool GetIgnoreViewFolder() const { return m_ignoreViewFolder; }
	void SetIgnoreViewFolder(bool ignore) { m_ignoreViewFolder = ignore; }

	bool GetIgnoreNetworkStatus() const { return m_ignoreNetworkStatus; }
	void SetIgnoreNetworkStatus(bool ignore) { m_ignoreNetworkStatus = ignore; }

	int GetConnectTimeout() const { return m_connectTimeout; }
	void SetConnectTimeout(int timeout) { m_connectTimeout = timeout; }

	bool GetCleanDisconnect() const { return m_cleanDisconnect; }
	void SetCleanDisconnect(bool clean) { m_cleanDisconnect = clean; }

	bool GetEnableCompress() const { return m_enableCompress; }
	void SetEnableCompress(bool compress) { m_enableCompress = compress; }

	int GetSessionKeepAlive() const { return m_sessionKeepAlive; }
	void SetSessionKeepAlive(int sessionKeepAlive) { m_sessionKeepAlive = sessionKeepAlive; }

	int GetKeepAliveForSync() const { return m_keepAliveForSync; }
	void SetKeepAliveForSync(bool keepAliveForSync) { m_keepAliveForSync = keepAliveForSync; }

	int GetNumAutoDownloadBodies() const { return m_numAutoDownloadBodies; }
	void SetNumAutoDownloadBodies(int num) { m_numAutoDownloadBodies = num; }

	static const int DEFAULT_INACTIVITY_TIMEOUT;
	static const int DEFAULT_HEADER_BATCH_SIZE;
	static const int DEFAULT_MAX_EMAILS;
	static const int DEFAULT_CONNECT_TIMEOUT;
	static const int DEFAULT_SESSION_KEEPALIVE;
	static const int DEFAULT_NUM_AUTODOWNLOAD_BODIES;

protected:
	void GetOptionalInt(const MojObject& obj, const char* prop, int& value, int min, int max);
	void GetOptionalBool(const MojObject& obj, const char* prop, bool& value);

	// Number of seconds before the process should shut down if nothing is active.
	// Zero for no timeout
	int	m_inactivityTimeout;

	// Max headers to download in one request
	int m_headerBatchSize;

	// Max emails to keep locally on the device per folder
	int m_maxEmails;

	// Ignore requests to sync when viewing a folder in the UI
	bool m_ignoreViewFolder;

	// Ignore network status
	bool m_ignoreNetworkStatus;

	// Connect timeout in seconds
	int m_connectTimeout;

	// Whether to send LOGOUT when disconnecting
	bool m_cleanDisconnect;

	// Enable compression
	bool m_enableCompress;

	// How long to keep alive inactive sessions in seconds
	// Zero disconnect immediately
	int m_sessionKeepAlive;

	// Whether to stay connected to the server while waiting for the next sync interval
	bool m_keepAliveForSync;

	// How many bodies to automatically download per folder
	int m_numAutoDownloadBodies;

	static ImapConfig s_instance;
};

#endif /* IMAPCONFIG_H_ */
