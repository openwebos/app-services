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


#include "data/SmtpAccount.h"

using namespace std;

SmtpAccount::SmtpAccount()
: m_port(0),
  m_useSmtpAuth(true),
  m_encryption(Encrypt_None)
{
}

SmtpAccount::~SmtpAccount()
{
}

const MojObject& SmtpAccount::GetAccountId() const
{
	return m_accountId;
}

void SmtpAccount::SetAccountId(const MojObject& id)
{
	m_accountId = id;
}

const string& SmtpAccount::GetHostname() const
{
	return m_hostname;
}

void SmtpAccount::SetHostname(const char* hostname)
{
	m_hostname = hostname;
}

const string& SmtpAccount::GetUsername() const
{
	return m_username;
}

void SmtpAccount::SetUsername(const char* username)
{
	m_username = username;
}

const string& SmtpAccount::GetPassword() const
{
	return m_password;
}

void SmtpAccount::SetPassword(const char* password)
{
	m_password = password;
}

bool SmtpAccount::UseSmtpAuth() const
{
	return m_useSmtpAuth;
}

void SmtpAccount::SetUseSmtpAuth(bool useSmtpAuth)
{
	m_useSmtpAuth = useSmtpAuth;
}

const string& SmtpAccount::GetYahooToken() const
{
	return m_yahooToken;
}

void SmtpAccount::SetYahooToken(const char* yahooToken)
{
	m_yahooToken = yahooToken;
}

const string& SmtpAccount::GetEmail() const
{
	return m_email;
}

void SmtpAccount::SetEmail(const char* email)
{
	m_email = email;
}

int SmtpAccount::GetPort() const
{
	return m_port;
}

void SmtpAccount::SetPort(int port)
{
	m_port = port;
}

SmtpAccount::EncryptionType SmtpAccount::GetEncryption() const
{
	return m_encryption;
}

void SmtpAccount::SetEncryption(const EncryptionType encryption)
{
	m_encryption = encryption;
}
