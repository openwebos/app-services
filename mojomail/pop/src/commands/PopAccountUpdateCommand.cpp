// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#include "commands/PopAccountUpdateCommand.h"
#include "activity/ActivityBuilderFactory.h"
#include "data/PopAccountAdapter.h"

PopAccountUpdateCommand::PopAccountUpdateCommand(PopClient& client, MojObject& payload, bool credentialsChanged)
: PopClientCommand(client, "Update POP account watches"),
  m_payload(payload),
  m_credentialsChanged(credentialsChanged),
  m_getAccountSlot(this, &PopAccountUpdateCommand::GetAccountResponse),
  m_getPasswordSlot(this, &PopAccountUpdateCommand::GetPasswordResponse),
  m_createActivitySlot(this, &PopAccountUpdateCommand::CreateActivityResponse),
  m_notifySmtpSlot(this, &PopAccountUpdateCommand::NotifySmtpResponse),
  m_callSyncFolderOnClient(false),
  m_activityBuilderFactory(new ActivityBuilderFactory())
{

}

PopAccountUpdateCommand::~PopAccountUpdateCommand()
{

}

void PopAccountUpdateCommand::RunImpl()
{
	MojString json;
	MojErr err = m_payload.toJson(json);
	ErrorToException(err);
	MojLogDebug(m_log, "PopAccountUpdateCommand with payload %s", json.data());

	m_accountId = m_client.GetAccountId();
	m_activityBuilderFactory->SetAccountId(m_accountId);
	m_client.GetDatabaseInterface().GetAccount(m_getAccountSlot, m_accountId);
}

MojErr PopAccountUpdateCommand::GetAccountResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);

		if (results.size() > 0 && results.at(0, m_transportObj)) {
			GetPassword();
		}
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountUpdateCommand::GetPassword()
{
	try {
		MojObject params;
		MojErr err = params.put("accountId", m_accountId);
		ErrorToException(err);

		err = params.putString("name", "common");
		ErrorToException(err);

		err = m_client.CreateRequest()->send(m_getPasswordSlot, "com.palm.service.accounts","readCredentials", params);
		ErrorToException(err);
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}
}

MojErr PopAccountUpdateCommand::GetPasswordResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check the result first
		ErrorToException(err);

		MojObject credentials;
		err = response.getRequired("credentials", credentials);
		ErrorToException(err);

		MojString password;
		err = credentials.getRequired("password", password);
		ErrorToException(err);

		boost::shared_ptr<PopAccount> newPopAccount(new PopAccount());
		PopAccountAdapter::GetPopAccountFromTransportObject(m_transportObj, *(newPopAccount.get()));
		newPopAccount->SetPassword(password.data());

		boost::shared_ptr<PopAccount> oldPopAccount = m_client.GetAccount();

		if (!oldPopAccount.get()) {
			m_callSyncFolderOnClient = true;
		} else if (newPopAccount->GetPort() != oldPopAccount->GetPort() ||
			newPopAccount->GetEncryption() != oldPopAccount->GetEncryption() ||
			newPopAccount->GetUsername() != oldPopAccount->GetUsername() ||
			newPopAccount->GetSyncFrequencyMins() != oldPopAccount->GetSyncWindowDays() ||
			newPopAccount->GetHostName() != oldPopAccount->GetHostName()) {
			m_callSyncFolderOnClient = true;
		}
		
		// Should be initial sync if the old account does not exist 
		// or the old account is in initial sync state.
		newPopAccount->SetInitialSync(!oldPopAccount.get() || oldPopAccount->IsInitialSync());
		m_client.SetAccount(newPopAccount);
		UpdateAccountWatchActivity();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountUpdateCommand::UpdateAccountWatchActivity()
{
	CommandTraceFunction();

	try {
		MojObject revPop;
		MojErr err = m_transportObj.getRequired("_revPop", revPop);
		ErrorToException(err);

		ActivityBuilder ab;
		m_activityBuilderFactory->BuildAccountPrefsWatch(ab, revPop);

		MojObject payload;
		err = payload.put("activity", ab.GetActivityObject());
		ErrorToException(err);
		err = payload.put("start", true);
		ErrorToException(err);
		err = payload.put("replace", true);
		ErrorToException(err);

		m_client.SendRequest(m_createActivitySlot, "com.palm.activitymanager", "create", payload);
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}
}

MojErr PopAccountUpdateCommand::CreateActivityResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	try {
		ErrorToException(err);

		MojString json;
		err = response.toJson(json);
		ErrorToException(err);

		Sync();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopAccountUpdateCommand::Sync()
{
	try {
		if (m_callSyncFolderOnClient) {
			// remove activity from payload & pass no activity to SyncAccount
			bool activityFound = false;
			MojErr err = m_payload.del("$activity", activityFound);

			if (err == MojErrNone) {
				err = m_payload.putBool("force", true);
				ErrorToException(err);
				m_client.SyncAccount(m_payload);
			}
		}

		if(m_credentialsChanged) {
			NotifySmtp();
		} else {
			Done();
		}
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception", __FILE__, __LINE__));
	}
}

//  Tell SMTP that the account has been updated; only if the credentials changed
void PopAccountUpdateCommand::NotifySmtp()
{
	CommandTraceFunction();

	MojErr err;
	MojObject payload;

	err = payload.put("accountId", m_client.GetAccountId());
	ErrorToException(err);

	m_client.SendRequest(m_notifySmtpSlot, "com.palm.smtp", "credentialsChanged", payload);
}

MojErr PopAccountUpdateCommand::NotifySmtpResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);
	} catch(const std::exception& e) {
		// log and ignore error
		MojLogError(m_log, "error calling SMTP credentialsChanged: %s", e.what());
	}

	Done();

	return MojErrNone;
}

void PopAccountUpdateCommand::Done()
{
	Complete();
}
