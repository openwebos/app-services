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

#ifndef SMTPACCOUNTADAPTER_H_
#define SMTPACCOUNTADAPTER_H_

#include "core/MojObject.h"
#include "data/SmtpAccount.h"
#include "boost/shared_ptr.hpp"

/**
 * This class is responsible for converting between SmtpAccount and MojObject.
 */
class SmtpAccountAdapter
{
public:

	static const char* const KIND;
	static const char* const SYNCKEY;
	static const char* const ID;
	static const char* const REV;
	static const char* const ACCOUNT_ID;
	static const char* const SCHEMA;
	static const char* const USERNAME;
	static const char* const EMAIL;
	static const char* const SERVER;
	static const char* const PORT;
	static const char* const ENCRYPTION;
	static const char* const CONFIG;
	static const char* const SMTPCONFIG;
	static const char* const OUTBOX_FOLDERID;
	static const char* const NO_SSL;
	static const char* const REQUIRE_SSL;
	static const char* const TLS;
	static const char* const TLS_IF_AVAILABLE;
	static const char* const DOMAIN;
	static const char* const PASSWORD;
	static const char* const USE_SMTP_AUTH;

	virtual ~SmtpAccountAdapter() { };
	static void GetSmtpAccount(const MojObject& in, const MojObject& transportIn, boost::shared_ptr<SmtpAccount>& out);
	static void GetSmtpConfigFromPayload(const MojObject& payload, MojObject& smtpAccount);
	static void GetSmtpAccountFromPayload(const MojObject& payload, MojObject& smtpAccount);
	static void GetAccountFromValidationPayload(const MojObject& payload, const MojObject& persistedAccount, boost::shared_ptr<SmtpAccount>& out );

protected:
	SmtpAccountAdapter() { };
	static MojObject GetFolderId(const MojObject& obj, const char* prop);

};

#endif /* SMTPACCOUNTADAPTER_H_ */
