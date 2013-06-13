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

#ifndef IMAPACCOUNT_H_
#define IMAPACCOUNT_H_

#include "core/MojObject.h"
#include "data/EmailAccount.h"
#include "data/ImapLoginSettings.h"
#include <string>
#include "exceptions/MailException.h"
#include <boost/shared_ptr.hpp>

/**
 * \brief
 * An account contains a url, domain, credentials, e-mail address, and the folder list synchronization key.
 */
class ImapAccount : public EmailAccount
{
public:
	/**
	 * If the sync frequency value is set to this the account will use the PING command to monitor changes on the server.
	 */
	static const int AS_ITEMS_ARRIVE = -1;

	/**
	 * If the sync frequency value is set to this the account will only be sync'd when the user requests it.
	 */
	static const int MANUAL = 0;

	enum EncryptionType{
			Encrypt_None,
			Encrypt_SSL,
			Encrypt_TLS,
			Encrypt_TLSIfAvailable
	};

	ImapAccount();
	virtual ~ImapAccount();

	// Aliases for GetAccountId() and SetAccountId()
	const MojObject& GetId() const { return GetAccountId(); }
	void SetId(const MojObject& id) { SetAccountId(id); }

	const std::string& GetTemplateId() const;
	void SetTemplateId(const char* templateId);

	bool IsYahoo();
	bool IsGoogle();

	void SetLoginSettings(const boost::shared_ptr<ImapLoginSettings>& loginSettings)
	{
		m_loginSettings = loginSettings;
	}

	bool HasLoginSettings() const { return m_loginSettings.get(); }

	bool HasPassword() { return m_hasPassword; }

	void SetHasPassword(bool hasPassword) { m_hasPassword = hasPassword;}

	const ImapLoginSettings& GetLoginSettings() const
	{
		if(m_loginSettings.get())
			return *m_loginSettings;
		else
			throw MailException("no login settings available", __FILE__, __LINE__);
	}

	const std::string& GetNamespaceRoot() const { return m_namespaceRoot; }
	void SetNamespaceRoot(const std::string& root) { m_namespaceRoot = root; }

	bool GetEnableCompression() const { return m_compress; }
	void SetEnableCompression(bool enable) { m_compress = enable; }

	bool GetEnableExpunge() const { return m_expunge; }
	void SetEnableExpunge(bool enable) { m_expunge = enable; }

private:
	std::string m_templateId;
	std::string m_email;

	std::string	m_namespaceRoot;

	bool		m_compress;
	bool		m_expunge;
	bool 		m_hasPassword;
	
	boost::shared_ptr<ImapLoginSettings>	m_loginSettings;
};

#endif /* IMAPACCOUNT_H_ */
