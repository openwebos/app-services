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


#include "data/SmtpAccountAdapter.h"
#include "SmtpCommon.h"

const char* const SmtpAccountAdapter::KIND 	 				= "_kind";
const char* const SmtpAccountAdapter::SYNCKEY	 			= "syncKey";
const char* const SmtpAccountAdapter::ID		 			= "_id";
const char* const SmtpAccountAdapter::REV					= "_rev";
const char* const SmtpAccountAdapter::ACCOUNT_ID			= "accountId";

const char* const SmtpAccountAdapter::SCHEMA				= "com.palm.smtp.account:1";
const char* const SmtpAccountAdapter::USERNAME 				= "username";
const char* const SmtpAccountAdapter::EMAIL 				= "email";
const char* const SmtpAccountAdapter::SERVER				= "server";
const char* const SmtpAccountAdapter::PORT 					= "port";
const char* const SmtpAccountAdapter::ENCRYPTION	 		= "encryption";
const char* const SmtpAccountAdapter::CONFIG 				= "config";
const char* const SmtpAccountAdapter::DOMAINN				= "domain";
const char* const SmtpAccountAdapter::PASSWORD				= "password";
const char* const SmtpAccountAdapter::USE_SMTP_AUTH			= "useSmtpAuth";

const char* const SmtpAccountAdapter::OUTBOX_FOLDERID		= "outboxFolderId";

const char* const SmtpAccountAdapter::NO_SSL 				= "none";
const char* const SmtpAccountAdapter::REQUIRE_SSL 			= "ssl";
const char* const SmtpAccountAdapter::TLS 					= "tls";
const char* const SmtpAccountAdapter::TLS_IF_AVAILABLE 		= "tlsIfAvailable";

// Used for normal account/credentials loading
void SmtpAccountAdapter::GetSmtpAccount(const MojObject& in, const MojObject& transportIn, boost::shared_ptr<SmtpAccount>& out)
{
            
	MojObject id;
	MojErr err = transportIn.getRequired(ACCOUNT_ID, id); // should be in
	ErrorToException(err);
	out->SetAccountId(id);
	
	bool hasTemplateId = false;
	MojString templateId;
	err = in.get("templateId", templateId, hasTemplateId); // FIXME use constant
	ErrorToException(err);

	if(hasTemplateId) {
		out->SetTemplateId( string(templateId.data()) );
	}

	MojObject smtpConfig;
	err = transportIn.getRequired("smtpConfig", smtpConfig);
	ErrorToException(err);
	
	MojString username;
	// try getting the username from the smtpConfig object
	// if it doesn't exist check the transport object
	// if that doesn't exist, it must exist in the account object
	if (smtpConfig.getRequired(USERNAME, username) != MojErrNone) {
		if (transportIn.getRequired(USERNAME, username) != MojErrNone) {
			err = in.getRequired(USERNAME, username);
			ErrorToException(err);
		}
	}
	out->SetUsername(username.data());

	MojString server;
	err = smtpConfig.getRequired(SERVER, server);
	ErrorToException(err);
	out->SetHostname(server.data());

	int port;
	err = smtpConfig.getRequired(PORT, port);
	ErrorToException(err);
	out->SetPort(port);

	MojString encryptionStr;
	err = smtpConfig.getRequired(ENCRYPTION, encryptionStr);
	ErrorToException(err);

	// Optional useSmtpAuth (default true)
	bool useSmtpAuth = true;
	if(smtpConfig.get(USE_SMTP_AUTH, useSmtpAuth)) {
		out->SetUseSmtpAuth(useSmtpAuth);
	}

	if (encryptionStr == "none")
		out->SetEncryption(SmtpAccount::Encrypt_None);
	else if(encryptionStr == "ssl")
		out->SetEncryption(SmtpAccount::Encrypt_SSL);
	else if(encryptionStr == "tls")
		out->SetEncryption(SmtpAccount::Encrypt_TLS);
	else if(encryptionStr == "tlsIfAvailable")
		out->SetEncryption(SmtpAccount::Encrypt_TLSIfAvailable);
	else {
			// TODO: Make Real Error
			ErrorToException(MojErrInvalidArg);
	}

	// Retrieve the optional error status
	MojObject error;
	if(smtpConfig.get("error", error))
	{
		EmailAccount::AccountError accErr;
		MojInt64 errorCode;
		if(error.get("errorCode", errorCode))
		{
			accErr.errorCode = (MailError::ErrorCode)errorCode;
		}

		bool found = false;
		MojString errorText;
		err = error.get("errorText", errorText, found);
		ErrorToException(err);
		if(found)
		{
			accErr.errorText = errorText.data();
		}

		out->SetSmtpError(accErr);
	}
	
}

// Used for account creation
void SmtpAccountAdapter::GetSmtpConfigFromPayload(const MojObject& payload, MojObject& smtpAccount)
{
	MojString email;
	bool hasEmail = false;
	MojErr err = payload.get(EMAIL, email, hasEmail);
	ErrorToException(err);
	if(hasEmail)
	{
	   err = smtpAccount.put(EMAIL, email);
	   ErrorToException(err);
	}

	MojObject encryption;
	err = payload.getRequired(ENCRYPTION, encryption);
	ErrorToException(err);
	err = smtpAccount.put(ENCRYPTION, encryption);
	ErrorToException(err);

	MojObject port;
	err = payload.getRequired(PORT, port);
	ErrorToException(err);
	err = smtpAccount.put(PORT, port);
	ErrorToException(err);

	MojObject server;
	err = payload.getRequired(SERVER, server);
	ErrorToException(err);
	err = smtpAccount.put(SERVER, server);
	ErrorToException(err);

	MojString username;
	bool hasUsername = false;
	err = payload.get(USERNAME, username, hasUsername);
	ErrorToException(err);
	if(hasUsername)
	{
	   err = smtpAccount.put(USERNAME, username);
	   ErrorToException(err);
	}

	// Optional useSmtpAuth
	bool useSmtpAuth = true;
	if(payload.get(USE_SMTP_AUTH, useSmtpAuth)) {
		err = smtpAccount.put(USE_SMTP_AUTH, useSmtpAuth);
		ErrorToException(err);
	}
}

// Used for account validation
void SmtpAccountAdapter::GetAccountFromValidationPayload(const MojObject& payload, const MojObject& persistedAccount, boost::shared_ptr<SmtpAccount>& out )
{
	/* Payload looks like this:
	 *
	 * {"config":{"domain":"foo.com","email":"a@foo.com","encryption":"none",
	 *            "port":25,"server":"smtp.foo.com","username":"a@foo.com"},
	 *  "password":"secret",
	 *  "username":"a@foo.com"}
	 */

	MojErr err;
	// First, if the persisted data exist, load it to the output account object.
	if(!persistedAccount.empty()) {
		GetSmtpAccount(payload, persistedAccount, out);
	}

	// Overwrite the values for the input object if exists.
	MojObject config;
	bool found;
	found = payload.get("config", config);

	if(found) {
		MojString hostname;
		err = config.get("server", hostname, found);
		ErrorToException(err);
		if(found) {
			out->SetHostname(hostname.data());
		}

		int port;
		err = config.get("port", port, found);
		ErrorToException(err);
		if(found) {
			out->SetPort(port);
		}

		MojString username;
		err = config.get("username", username, found); // username in config, if present, wins
		ErrorToException(err);
		if (!found) {
			err = payload.get("username", username, found);
			ErrorToException(err);
		}
		if(found) {
			out->SetUsername(username.data());
		}

		MojString password;
		err = payload.get("password", password, found);
		ErrorToException(err);
		if(found) {
			out->SetPassword(password.data());
		}

		// FIXME: Email may not be used by SMTP, look at purging
		MojString email;
		err = config.get("email", email, found); // email in config, if present, wins
		ErrorToException(err);
		if (!found) {
			err = payload.get("username", email, found);
			ErrorToException(err);
		}
		if(found) {
			out->SetEmail(email.data());
		}

		// Optional useSmtpAuth (default true)
		bool useSmtpAuth = true;
		if(config.get(USE_SMTP_AUTH, useSmtpAuth)) {
			out->SetUseSmtpAuth(useSmtpAuth);
		}

		MojString encryptionStr;
		err = config.get("encryption", encryptionStr, found);
		ErrorToException(err);

		if(found) {
			if (encryptionStr == "none")
				out->SetEncryption(SmtpAccount::Encrypt_None);
			else if(encryptionStr == "ssl")
				out->SetEncryption(SmtpAccount::Encrypt_SSL);
			else if(encryptionStr == "tls")
				out->SetEncryption(SmtpAccount::Encrypt_TLS);
			else if(encryptionStr == "tlsIfAvailable")
				out->SetEncryption(SmtpAccount::Encrypt_TLSIfAvailable);
			else {
				// TODO: Make Real Error
				ErrorToException(MojErrInvalidArg);
			}
		}
	}
}
