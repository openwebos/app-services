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


#ifndef IMAPACCOUNTADAPTER_H_
#define IMAPACCOUNTADAPTER_H_

#include "core/MojObject.h"
#include "data/ImapAccount.h"

/**
 * This class is reponsible for converting between ImapAccount and MojObject.
 */
class ImapAccountAdapter
{
public:

	static const char* const KIND;
	static const char* const SYNCKEY;
	static const char* const ID;
	static const char* const TEMPLATEID;
	static const char* const ACCOUNT_ID;
	static const char* const SCHEMA;
	static const char* const USERNAME;
	static const char* const EMAIL;
	static const char* const HOSTNAME;
	static const char* const PORT;
	static const char* const ENCRYPTION;
	static const char* const CONFIG;

	static const char* const REV;
	static const char* const CONFIG_REV;

	static const char* const SYNC_FREQUENCY_MIN;
	static const char* const SYNC_WINDOW_DAYS;

	static const char* const NO_SSL;
	static const char* const REQUIRE_SSL;
	static const char* const TLS;
	static const char* const TLS_IF_AVAILABLE;

	// IMAP
	static const char* const ROOT_FOLDER;

	// Experimental
	static const char* const ENABLE_COMPRESS;
	static const char* const ENABLE_EXPUNGE;

	static const int DEFAULT_SYNC_FREQUENCY_MINS;
	static const int DEFAULT_SYNC_WINDOW_DAYS;

	virtual ~ImapAccountAdapter() { };
	static void GetImapAccount(const MojObject& in, const MojObject& transportIn, ImapAccount& out);
	static void GetImapAccountFromPayload(const MojObject& accountObj, const MojObject& payload, MojObject& imapAccount);
	static ImapLoginSettings::EncryptionType GetEncryptionType(const MojString& encryptionStr);

	static void ParseLoginConfig(const MojObject& config, ImapLoginSettings& settings);

protected:
	ImapAccountAdapter() { };
	
	static MojObject GetFolderId(const MojObject& obj, const char* prop);
};

#endif /* IMAPACCOUNTADAPTER_H_ */
