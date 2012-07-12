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

#ifndef IMAPLOGINSETTINGS_H_
#define IMAPLOGINSETTINGS_H_

#include <string>

class ImapLoginSettings
{
public:
	enum EncryptionType {
		Encrypt_None,
		Encrypt_SSL,
		Encrypt_TLS,
		Encrypt_TLSIfAvailable
	};

	ImapLoginSettings();
	virtual ~ImapLoginSettings();

	const std::string& GetHostname() const { return m_hostname; }
	void SetHostname(const char* hostname) { m_hostname = hostname; }

	int GetPort() const { return m_port; }
	void SetPort(int port) { m_port = port; }

	EncryptionType GetEncryption() const { return m_encryption; }
	void SetEncryption(EncryptionType type) { m_encryption = type; }

	const std::string& GetUsername() const { return m_username; }
	void SetUsername(const char* username) { m_username = username; }

	const std::string& GetPassword() const { return m_password; }
	void SetPassword(const char* password) { m_password = password; }

	const std::string& GetYahooToken() const { return m_yahooToken; }
	void SetYahooToken(const char* yahooToken) { m_yahooToken = yahooToken; }

protected:
	std::string		m_hostname;
	int				m_port;
	EncryptionType	m_encryption;

	std::string		m_username;
	std::string		m_password;
	std::string		m_yahooToken;
};

#endif /* IMAPLOGINSETTINGS_H_ */
