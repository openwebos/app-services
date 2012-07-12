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

#ifndef SMTPACCOUNT_H_
#define SMTPACCOUNT_H_

#include "core/MojObject.h"
#include "data/EmailAccount.h"
#include <string>

/**
 * \brief
 * An account contains a url, domain, credentials, e-mail address, and the folder list synchronization key.
 */
class SmtpAccount
{
public:

	// Distinct from Error routines in Email account.
	const EmailAccount::AccountError GetSmtpError() const { return m_smtpError; }
	void SetSmtpError(const EmailAccount::AccountError& error) { m_smtpError = error; }

	enum EncryptionType {
			Encrypt_None,
			Encrypt_SSL,
			Encrypt_TLS,
			Encrypt_TLSIfAvailable
	};

	SmtpAccount();
	virtual ~SmtpAccount();

	const MojObject& GetAccountId() const;
	void SetAccountId(const MojObject& id);

	const std::string& GetHostname() const;
	void SetHostname(const char* url);

	const std::string& GetUsername() const;
	void SetUsername(const char* username);

	const std::string& GetPassword() const;
	void SetPassword(const char* password);
	
	bool UseSmtpAuth() const;
	void SetUseSmtpAuth(bool useSmtpAuth);

	const std::string& GetYahooToken() const;
	void SetYahooToken(const char * token);

	const std::string& GetEmail() const;
	void SetEmail(const char* email);

	void SetTemplateId(const std::string& templateId) { m_templateId = templateId; }
	const std::string& GetTemplateId() const { return m_templateId; }

	int GetPort() const;
	void SetPort(int port);

	EncryptionType GetEncryption() const;
	void SetEncryption(EncryptionType e);
	
	bool NeedsSSL() const { return GetEncryption() == Encrypt_SSL; }
	bool WantsTLS() const { return GetEncryption() == Encrypt_TLS || GetEncryption() == Encrypt_TLSIfAvailable; }
	bool NeedsTLS() const { return GetEncryption() == Encrypt_TLS; }
	
	// TODO move to a base account class

	const MojObject& GetOutboxFolderId() const { return m_outboxFolderId; }

	void SetOutboxFolderId(const MojObject& id) { m_outboxFolderId = id; }


private:
	MojObject	m_accountId;
	std::string m_hostname;
	int 		m_port;
	std::string m_username;
	std::string m_password;
	bool		m_useSmtpAuth;
	std::string m_yahooToken;
	std::string m_email;
	EncryptionType m_encryption;

	std::string m_templateId;

	MojObject	m_outboxFolderId;
	EmailAccount::AccountError m_smtpError;

};

#endif /* SMTPACCOUNT_H_ */
