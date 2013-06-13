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

#include "commands/ImapAccountFinder.h"
#include "data/DatabaseInterface.h"
#include "data/DatabaseAdapter.h"
#include "data/ImapAccountAdapter.h"
#include "ImapClient.h"
#include "ImapPrivate.h"

using namespace std;

ImapAccountFinder::ImapAccountFinder(ImapClient& client,
								   MojObject accountId,
								   bool sync)
: ImapCommand(client),
  m_client(client),
  m_account(new ImapAccount()),
  m_accountId(accountId),
  m_sync(sync),
  m_getAccountSlot(this, &ImapAccountFinder::GetAccountResponse),
  m_getPasswordSlot(this, &ImapAccountFinder::GetPasswordResponse),
  m_getImapAccountSlot(this, &ImapAccountFinder::GetImapAccountResponse)
{
	m_account->SetAccountId(accountId);
}

ImapAccountFinder::~ImapAccountFinder()
{

}

/**
 * Runs the ImapAccountFinder command. Flow essentially pulls the com.palm.account obj for the id,
 * the com.palm.account.credentials obj for the account login info, and the actual email settings 
 * from com.palm.imap.account. Once all three have been retrieved, updates the client with the
 * fully constructed IMAP account.
 */
void ImapAccountFinder::RunImpl()
{
	CommandTraceFunction();

	GetAccount();
}

void ImapAccountFinder::GetAccount()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "getting account %s info from account service and database", AsJsonString(m_accountId).c_str());

	MojObject params;
	MojErr err = params.put("accountId", m_accountId);
	ErrorToException(err);

	m_client.SendRequest(m_getAccountSlot, "com.palm.service.accounts","getAccountInfo", params);
}

MojErr ImapAccountFinder::GetAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	
	try {
		// check the result first
		ResponseToException(response, err);

		err = response.getRequired("result", m_accountObj);
		ErrorToException(err);

		// Get IMAP account object
		GetImapAccount();

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void ImapAccountFinder::GetImapAccount()
{
	CommandTraceFunction();

	m_client.GetDatabaseInterface().GetAccount(m_getImapAccountSlot, m_accountId);
}

MojErr ImapAccountFinder::GetImapAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check database result first
		ResponseToException(response, err);

		MojObject transportObj;

		try {
			DatabaseAdapter::GetOneResult(response, transportObj);
		} catch(const exception& e) {
			MojLogError(m_log, "error loading IMAP preferences for account %s from database: %s", AsJsonString(m_accountId).c_str(), e.what());

			// Rethrow exception
			throw;
		}

		ImapAccountAdapter::GetImapAccount(m_accountObj, transportObj, *(m_account.get()));

		// Get password
		GetPassword();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void ImapAccountFinder::GetPassword()
{
	CommandTraceFunction();

	MojObject params;
	MojErr err = params.put("accountId", m_accountId);
	ErrorToException(err);

	err = params.putString("name", "common");
	ErrorToException(err);

	err = m_client.CreateRequest()->send(m_getPasswordSlot, "com.palm.service.accounts","readCredentials", params);
	ErrorToException(err);
}

MojErr ImapAccountFinder::GetPasswordResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check the result first
		ResponseToException(response, err);

		// Copy login settings from existing login settings
		boost::shared_ptr<ImapLoginSettings> loginSettings(new ImapLoginSettings(m_account->GetLoginSettings()));

		MojObject credentials;
		if(response.get("credentials", credentials)) {
			// Optional password
			bool hasPassword = false;
			MojString password;
			err = credentials.get("password", password, hasPassword);
			ErrorToException(err);
			if(hasPassword) {
				loginSettings->SetPassword(password.data());
			}

			// Optional security token for Yahoo
			bool hasSecurityToken = false;
			MojString securityToken;
			err = credentials.get("securityToken", securityToken, hasSecurityToken);
			ErrorToException(err);
			if(hasSecurityToken) {
				loginSettings->SetYahooToken(securityToken.data());
			}

			// Update login settings
			m_account->SetHasPassword(!password.empty() || !securityToken.empty());

		} else {
			MojLogCritical(m_log, "no credentials available for account %s", AsJsonString(m_accountId).c_str());
		}

		// Update login settings
		m_account->SetLoginSettings(loginSettings);
		
		Done();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void ImapAccountFinder::Done()
{
	// all done!
	m_client.SetAccount(m_account);
	Complete();
}

void ImapAccountFinder::Failure(const exception& e)
{
	ImapCommand::Failure(e);

	boost::shared_ptr<ImapAccount> nullAccount;

	m_client.SetAccount(nullAccount);
}
