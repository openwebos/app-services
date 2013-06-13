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

#include "data/ImapAccountAdapter.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailAccountAdapter.h"
#include "ImapCoreDefs.h"
#include "ImapConfig.h"

using namespace std;

const char* const ImapAccountAdapter::KIND 	 				= "_kind";
const char* const ImapAccountAdapter::SYNCKEY	 			= "syncKey";
const char* const ImapAccountAdapter::ID		 			= "_id";
const char* const ImapAccountAdapter::TEMPLATEID		 	= "templateId";
const char* const ImapAccountAdapter::ACCOUNT_ID			= "accountId";
const char* const ImapAccountAdapter::REV		 			= "_rev";

const char* const ImapAccountAdapter::CONFIG_REV			= "ImapConfigRev";

const char* const ImapAccountAdapter::SCHEMA				= "com.palm.imap.account:1";
const char* const ImapAccountAdapter::USERNAME 				= "username";
const char* const ImapAccountAdapter::EMAIL 				= "email";
const char* const ImapAccountAdapter::HOSTNAME				= "server";
const char* const ImapAccountAdapter::PORT 					= "port";
const char* const ImapAccountAdapter::ENCRYPTION	 		= "encryption";
const char* const ImapAccountAdapter::CONFIG 				= "config";

const char* const ImapAccountAdapter::NO_SSL 				= "none";
const char* const ImapAccountAdapter::REQUIRE_SSL 			= "ssl";
const char* const ImapAccountAdapter::TLS 					= "tls";
const char* const ImapAccountAdapter::TLS_IF_AVAILABLE 		= "tlsIfAvailable";

// IMAP-specific
const char* const ImapAccountAdapter::ROOT_FOLDER 			= "rootFolder";

// Compatibility options
const char* const ImapAccountAdapter::ENABLE_COMPRESS		= "enableCompress";
const char* const ImapAccountAdapter::ENABLE_EXPUNGE		= "enableExpunge";

const int ImapAccountAdapter::DEFAULT_SYNC_FREQUENCY_MINS = -1; // push
const int ImapAccountAdapter::DEFAULT_SYNC_WINDOW_DAYS = 7; // one week

/**
 * Parse two database objects into an ImapAccount
 *
 * @param in			com.palm.mail.account database object
 * @param transportIn	com.palm.imap.account database object
 * @param out			ImapAccount
 */
void ImapAccountAdapter::GetImapAccount(const MojObject& in, const MojObject& transportIn, ImapAccount& out)
{
	MojObject id;
	MojErr err = in.getRequired(ID, id);
	ErrorToException(err);
	out.SetId(id);

	// Template from account object
	MojString templateId;
	bool hasTemplateId = false;
	err = in.get(TEMPLATEID, templateId, hasTemplateId); // make required later
	ErrorToException(err);
	if(hasTemplateId)
	{
		out.SetTemplateId(templateId.data());
	}
	
	boost::shared_ptr<ImapLoginSettings> loginSettings(new ImapLoginSettings());

	// Username from account object (this is always the email address)
	MojString username;
	in.getRequired(USERNAME, username);
	ErrorToException(err);

	loginSettings->SetUsername(username.data());

	// Read server settings
	ParseLoginConfig(transportIn, *loginSettings);

	// Store updated server settings on account
	out.SetLoginSettings(loginSettings);

	int syncFrequencyMin;
	err = transportIn.getRequired(EmailAccountAdapter::SYNC_FREQUENCY_MINS, syncFrequencyMin);
	ErrorToException(err);
	out.SetSyncFrequencyMins(syncFrequencyMin);

	int syncWindowDays;
	err = transportIn.getRequired(EmailAccountAdapter::SYNC_WINDOW_DAYS, syncWindowDays);
	ErrorToException(err);
	out.SetSyncWindowDays(syncWindowDays);

	MojInt64 rev;
	err = transportIn.getRequired(REV, rev);
	ErrorToException(err);
	out.SetRevision(rev);

	// Retrieve the optional error status
	MojObject errorObj;
	if(transportIn.get(EmailAccountAdapter::ERROR, errorObj))
	{
		EmailAccount::AccountError accountError;
		EmailAccountAdapter::ParseErrorInfo(errorObj, accountError);

		out.SetError(accountError);
	}

	// Compatibility/workaround options
	bool enableCompress = DatabaseAdapter::GetOptionalBool(transportIn, ENABLE_COMPRESS, ImapConfig::GetConfig().GetEnableCompress());
	bool enableExpunge = DatabaseAdapter::GetOptionalBool(transportIn, ENABLE_EXPUNGE, true);

	out.SetEnableCompression(enableCompress);
	out.SetEnableExpunge(enableExpunge);

	// Read special folders
	EmailAccountAdapter::ParseSpecialFolders(transportIn, out);

	// Read retry status
	EmailAccountAdapter::ParseAccountRetryStatus(transportIn, out);

	// Root folder prefix
	bool hasRootFolder = false;
	MojString rootFolder;
	err = transportIn.get(ROOT_FOLDER, rootFolder, hasRootFolder);
	ErrorToException(err);

	if(hasRootFolder) {
		out.SetNamespaceRoot( string(rootFolder.data()) );
	}
}

ImapLoginSettings::EncryptionType ImapAccountAdapter::GetEncryptionType(const MojString& encryptionStr)
{
	if (encryptionStr == NO_SSL)
		return ImapLoginSettings::Encrypt_None;
	else if(encryptionStr == REQUIRE_SSL)
		return ImapLoginSettings::Encrypt_SSL;
	else if(encryptionStr == TLS)
		return ImapLoginSettings::Encrypt_TLS;
	else if(encryptionStr == TLS_IF_AVAILABLE)
		return ImapLoginSettings::Encrypt_TLSIfAvailable;
	else {
		throw MailException("unknown/invalid ssl encryption type", __FILE__, __LINE__);
	}
}

// Copy fields from createAccount payload into com.palm.imap.account object
void ImapAccountAdapter::GetImapAccountFromPayload(const MojObject& accountObj, const MojObject& payload, MojObject& imapAccount)
{
	MojErr err = imapAccount.putString(ImapAccountAdapter::KIND, ImapAccountAdapter::SCHEMA);
	ErrorToException(err);

	MojObject accountId;
	err = payload.getRequired(ImapAccountAdapter::ACCOUNT_ID, accountId);
	ErrorToException(err);

	err = imapAccount.put(ImapAccountAdapter::ACCOUNT_ID, accountId);
	ErrorToException(err);

	MojObject config;
	err = payload.getRequired(ImapAccountAdapter::CONFIG, config);
	ErrorToException(err);

	MojString username;
	bool hasUsername = false;
	err = config.get(ImapAccountAdapter::USERNAME, username, hasUsername);
	ErrorToException(err);
	if(!hasUsername)
	{
		err = accountObj.getRequired(ImapAccountAdapter::USERNAME, username);
		ErrorToException(err);
	}
	err = imapAccount.put(ImapAccountAdapter::USERNAME, username);
	ErrorToException(err);

	MojString hostname;
	err = config.getRequired(ImapAccountAdapter::HOSTNAME, hostname);
	ErrorToException(err);

	err = imapAccount.put(ImapAccountAdapter::HOSTNAME, hostname);
	ErrorToException(err);

	int port;
	err = config.getRequired(ImapAccountAdapter::PORT, port);
	ErrorToException(err);

	err = imapAccount.put(ImapAccountAdapter::PORT, port);
	ErrorToException(err);

	MojString encryption;
	err = config.getRequired(ImapAccountAdapter::ENCRYPTION, encryption);
	ErrorToException(err);

	err = imapAccount.put(ImapAccountAdapter::ENCRYPTION, encryption);
	ErrorToException(err);

	/* Optional config parameters; fill in with defaults if not provided */

	bool hasSyncFrequency = false;
	int syncFrequencyMins = 0;
	config.get(EmailAccountAdapter::SYNC_FREQUENCY_MINS, syncFrequencyMins, hasSyncFrequency);
	ErrorToException(err);

	err = imapAccount.putInt(EmailAccountAdapter::SYNC_FREQUENCY_MINS,
			hasSyncFrequency ? syncFrequencyMins : DEFAULT_SYNC_FREQUENCY_MINS);
	ErrorToException(err);

	bool hasSyncWindow = false;
	int syncWindow = 0;
	err = config.get(EmailAccountAdapter::SYNC_WINDOW_DAYS, syncWindow, hasSyncWindow);
	ErrorToException(err);

	err = imapAccount.putInt(EmailAccountAdapter::SYNC_WINDOW_DAYS,
			hasSyncWindow ? syncWindow : DEFAULT_SYNC_WINDOW_DAYS);
	ErrorToException(err);
}

MojObject ImapAccountAdapter::GetFolderId(const MojObject& obj, const char* prop)
{
	MojObject folderId(MojObject::Undefined);
	if(obj.contains(prop)) {
		MojErr err = obj.getRequired(prop, folderId);
		ErrorToException(err);
	}
	return folderId;
}

void ImapAccountAdapter::ParseLoginConfig(const MojObject& config, ImapLoginSettings& login)
{
	MojErr err;

	// Optional username; overrides account username if available
	bool hasServerUsername = false;
	MojString serverUsername;
	err = config.get(ImapAccountAdapter::USERNAME, serverUsername, hasServerUsername);
	ErrorToException(err);
	if(hasServerUsername) {
		login.SetUsername(serverUsername.data());
	}

	MojString hostname;
	err = config.getRequired(ImapAccountAdapter::HOSTNAME, hostname);
	ErrorToException(err);
	login.SetHostname(hostname.data());

	int port;
	err = config.getRequired(ImapAccountAdapter::PORT, port);
	ErrorToException(err);
	login.SetPort(port);

	MojString encryptionStr;
	err = config.getRequired(ImapAccountAdapter::ENCRYPTION, encryptionStr);
	ErrorToException(err);
	login.SetEncryption(GetEncryptionType(encryptionStr));
}
