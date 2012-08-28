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


#include "ImapBusDispatcher.h"
#include "core/MojServiceMessage.h"
#include "ImapServiceApp.h"
#include "data/ImapAccount.h"
#include "ImapCoreDefs.h"
#include "boost/shared_ptr.hpp"
#include "ImapValidator.h"
#include "data/ImapAccountAdapter.h"
#include "data/DatabaseInterface.h"
#include "ImapPrivate.h"
#include "client/DownloadListener.h"
#include "client/SearchRequest.h"
#include "activity/Activity.h"
#include "activity/ActivityParser.h"
#include "data/DatabaseAdapter.h"
#include "client/CommandManager.h"

ImapBusDispatcher::ImapBusDispatcher(ImapServiceApp& app)
: BusClient(&app.GetService(), "com.palm.imap"),
  m_app(app),
  m_log("com.palm.imap"),
  m_dbInterface(app.GetDbClient()),
  m_cleanupValidatorsCallbackId(0),
  m_inactivityCallbackId(0),
  m_commandManager(new CommandManager(1000)),
  m_networkStatusMonitor(new NetworkStatusMonitor(*this))
{
}

ImapBusDispatcher::~ImapBusDispatcher()
{
	if(m_cleanupValidatorsCallbackId > 0) {
		g_source_remove(m_cleanupValidatorsCallbackId);
	}

	if(m_inactivityCallbackId > 0) {
		g_source_remove(m_inactivityCallbackId);
	}
}

MojErr ImapBusDispatcher::InitHandler()
{
	RegisterMethods();

	return MojErrNone;
}

MojErr ImapBusDispatcher::RegisterMethods()
{
	MojErr err = addMethod("ping", (Callback) &ImapBusDispatcher::ping);
	MojErrCheck(err);
	
	err = addMethod("validateAccount", (Callback) &ImapBusDispatcher::ValidateAccount);
	MojErrCheck(err);

	err = addMethod("accountCreated", (Callback) &ImapBusDispatcher::AccountCreated);
	MojErrCheck(err);

	err = addMethod("accountEnabled", (Callback) &ImapBusDispatcher::AccountEnabled);
	MojErrCheck(err);

	err = addMethod("accountDeleted", (Callback) &ImapBusDispatcher::AccountDeleted);
	MojErrCheck(err);

	err = addMethod("accountUpdated", (Callback) &ImapBusDispatcher::AccountUpdated);
	MojErrCheck(err);

	err = addMethod("credentialsChanged", (Callback) &ImapBusDispatcher::CredentialsChanged);
	MojErrCheck(err);

	err = addMethod("status", (Callback) &ImapBusDispatcher::StatusRequest);
	MojErrCheck(err);

	err = addMethod("syncAccount", (Callback) &ImapBusDispatcher::SyncAccount);
	MojErrCheck(err);

	err = addMethod("syncFolder", (Callback) &ImapBusDispatcher::SyncFolder);
	MojErrCheck(err);

	err = addMethod("syncUpdate", (Callback) &ImapBusDispatcher::SyncUpdate);
	MojErrCheck(err);

	err = addMethod("downloadMessage", (Callback) &ImapBusDispatcher::DownloadMessage);
	MojErrCheck(err);

	err = addMethod("downloadAttachment", (Callback) &ImapBusDispatcher::DownloadAttachment);
	MojErrCheck(err);

	err = addMethod("searchFolder", (Callback) &ImapBusDispatcher::SearchFolder); // experimental
	MojErrCheck(err);

	err = addMethod("sentEmails", (Callback) &ImapBusDispatcher::SentEmails);
	MojErrCheck(err);

	err = addMethod("startIdle", (Callback) &ImapBusDispatcher::StartIdle);
	MojErrCheck(err);

	err = addMethod("wakeupIdle", (Callback) &ImapBusDispatcher::WakeupIdle);
	MojErrCheck(err);

	err = addMethod("draftSaved", (Callback) &ImapBusDispatcher::DraftSaved);
	MojErrCheck(err);

	// Debug method
	err = addMethod("magicWand", (Callback) &ImapBusDispatcher::MagicWand);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr ImapBusDispatcher::ping(MojServiceMessage* msg, MojObject& payload)
{
	MojErr err;
	
	err = msg->replySuccess();
	MojErrCheck(err);
	
	return MojErrNone;
}

MojErr ImapBusDispatcher::ValidateAccount(MojServiceMessage* msg, MojObject& payload)
{
	try {
		MojRefCountedPtr<ImapValidator> validator(new ImapValidator(*this, msg, payload));
		validator->SetBusClient(*this);
		validator->SetDatabase(m_dbInterface);

		m_validators.push_back(validator);

		validator->Validate();

		MojLogInfo(m_log, "starting validator %p; %d validators running", validator.get(), m_validators.size());

		CheckActive();
	} catch(const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}

	return MojErrNone;
}

void ImapBusDispatcher::ValidationDone(ImapValidator* validator)
{
	vector<ValidatorPtr>::iterator it;
	for(it = m_validators.begin(); it != m_validators.end(); ++it) {
		if(it->get() == validator) {
			m_completedValidators.push_back(*it);
			m_validators.erase(it);
			break;
		}
	}

	if(m_completedValidators.size() == 1) {
		m_cleanupValidatorsCallbackId = g_idle_add(&ImapBusDispatcher::CleanupValidatorsCallback, this);
	}

	MojLogInfo(m_log, "validator %p done; remaining: %d", validator, m_validators.size());

	CheckActive();
}

gboolean ImapBusDispatcher::CleanupValidatorsCallback(gpointer data)
{
	ImapBusDispatcher* disp = static_cast<ImapBusDispatcher*>(data);
	assert(data);

	disp->m_cleanupValidatorsCallbackId = 0;
	disp->m_completedValidators.clear();

	return false;
}

MojErr ImapBusDispatcher::AccountCreated(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {
		// Create the com.palm.imap.account object
		MojRefCountedPtr<AccountCreator> creator(new AccountCreator(*this, m_log, msg, payload));
		m_commandManager->RunCommand(creator);

		return MojErrNone;
	} catch (const std::exception& e){
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

MojErr ImapBusDispatcher::AccountEnabled(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);
	// Move along. Nothing to see here. (At least for now)

	MojErr err;
	MojObject accountId;

	err = payload.getRequired("accountId", accountId);
	MojErrCheck(err);

	bool enabled = false;
	err = payload.getRequired("enabled", enabled);
	MojErrCheck(err);

	MojLogNotice(m_log, "accountEnabled called for %s with enabled %s", AsJsonString(accountId).c_str(), enabled ? "true" : "false");

	ClientPtr client = GetOrCreateClient(accountId);

	if(enabled) {
		// This will kick off a sync
		client->EnableAccount(msg);

		// TODO: don't reply until we finish setting up account.
		// That way, account service is aware if account setup fails.
		//return msg->replySuccess();
	} else {
		client->DisableAccount(msg, payload);
	}

	CheckActive();

	return MojErrNone;
}


MojErr ImapBusDispatcher::AccountUpdated(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		MojLogNotice(m_log, "account updated for account %s", AsJsonString(accountId).c_str());

		ActivityPtr activity = GetActivity(payload);

		ClientPtr client = GetOrCreateClient(accountId);
		client->UpdateAccount(accountId, activity, false);

		err = msg->replySuccess();
		ErrorToException(err);
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	CheckActive();

	return MojErrNone;
}

MojErr ImapBusDispatcher::CredentialsChanged(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		MojLogNotice(m_log, "credentials changed for account %s", AsJsonString(accountId).c_str());

		ActivityPtr activity = GetActivity(payload);

		ClientPtr client = GetOrCreateClient(accountId);
		client->UpdateAccount(accountId, activity, true);

		err = msg->replySuccess();
		ErrorToException(err);
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	CheckActive();

	return MojErrNone;
}


MojErr ImapBusDispatcher::AccountDeleted(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);
	// Move along. Nothing to see here. (At least for now)

	MojErr err;
	MojObject accountId;

	err = payload.getRequired("accountId", accountId);
	MojErrCheck(err);

	ClientPtr client = GetOrCreateClient(accountId);

	MojLogNotice(m_log, "accountDeleted called for account %s", AsJsonString(accountId).c_str());

	client->DeleteAccount(msg, payload);

	CheckActive();

	return MojErrNone;
}

MojErr ImapBusDispatcher::SentEmails(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);
	MojErr err;
	MojObject accountId;

	try {

		err = payload.getRequired("accountId", accountId);
		MojErrCheck(err);

		ClientPtr client = GetOrCreateClient(accountId);

		SyncParams syncParams;
		ActivityPtr activity = GetActivity(payload);

		if(activity.get()) {
			syncParams.GetSyncActivities().push_back(activity);
		}

		client->CheckOutbox(syncParams);

		err = msg->replySuccess();
		ErrorToException(err);

		CheckActive();
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;
}

MojErr ImapBusDispatcher::DraftSaved(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);
	MojErr err;
	MojObject accountId;

	try {

		err = payload.getRequired("accountId", accountId);
		MojErrCheck(err);

		ClientPtr client = GetOrCreateClient(accountId);

		ActivityPtr activity = GetActivity(payload);
		if(activity.get()) {
			activity->SetPurpose(Activity::WatchActivity);
		}

		client->CheckDrafts(activity);

		err = msg->replySuccess();
		ErrorToException(err);

		CheckActive();
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;
}

ImapBusDispatcher::AccountCreator::AccountCreator(ImapBusDispatcher& dispatcher,
												 MojLogger& logger,
												 MojServiceMessage* message,
												 MojObject& payload)
: ImapBusDispatcherCommand(dispatcher, logger),
  m_message(message),
  m_payload(payload),
  m_getAccountSlot(this, &ImapBusDispatcher::AccountCreator::GetAccountResponse),
  m_persistAccountSlot(this, &ImapBusDispatcher::AccountCreator::PersistAccountResponse),
  m_smtpAccountCreatedSlot(this, &ImapBusDispatcher::AccountCreator::SmtpAccountCreatedResponse)
{

}

ImapBusDispatcher::AccountCreator::~AccountCreator()
{

}

void ImapBusDispatcher::AccountCreator::RunImpl()
{
	CommandTraceFunction();

	try {
		MojObject accountId;
		MojErr err = m_payload.getRequired("accountId", accountId);
		ErrorToException(err);

		MojLogInfo(m_log, "creating account %s", AsJsonString(accountId).c_str());

		MojObject params;
		err = params.put("accountId", accountId);
		ErrorToException(err);

		m_busDispatcher.SendRequest(m_getAccountSlot, "com.palm.service.accounts","getAccountInfo", params);
	} catch(const MojErrException& e) {
		MojLogError(m_log, "error creating account: %s", e.what());
		m_message->replyError(e.GetMojErr(), e.what());
	} catch(const exception& e) {
		MojLogError(m_log, "error creating account: %s", e.what());
		m_message->replyError(MojErrInternal, e.what());
	}
}

MojErr ImapBusDispatcher::AccountCreator::GetAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		// check the result first
		ErrorToException(err);

		MojObject accountObj;
		err = response.getRequired("result", accountObj);
		ErrorToException(err);

		MojObject imapAccount;
		ImapAccountAdapter::GetImapAccountFromPayload(accountObj, m_payload, imapAccount);

		err = m_busDispatcher.m_app.GetDbClient().put(m_persistAccountSlot, imapAccount);
		ErrorToException(err);
	} catch(const MojErrException& e) {
		MojLogError(m_log, "error creating account: %s", e.what());
		m_message->replyError(e.GetMojErr(), e.what());
		Failure(e);
	} catch(const exception& e) {
		MojLogError(m_log, "error creating account: %s", e.what());
		m_message->replyError(MojErrInternal, e.what());
		Failure(e);
	}

	return MojErrNone;
}


MojErr ImapBusDispatcher::AccountCreator::PersistAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		MojObject result;
		DatabaseAdapter::GetOneResult(response, result);
		MojObject mailAccountId;
		err = result.getRequired("id", mailAccountId);
		ErrorToException(err);
		err = m_payload.put("mailAccountId", mailAccountId);
		ErrorToException(err);

		MojObject accountId;
		err = m_payload.getRequired("accountId", accountId);
		ErrorToException(err);

		m_busDispatcher.SendRequest(m_smtpAccountCreatedSlot, "com.palm.smtp", "accountCreated", m_payload);

	} catch(const MojErrException& e) {
		MojLogError(m_log, "error creating account: %s", e.what());
		m_message->replyError(e.GetMojErr(), e.what());
		Failure(e);
	} catch(const exception& e) {
		MojLogError(m_log, "error creating account: %s", e.what());
		m_message->replyError(MojErrInternal, e.what());
		Failure(e);
	}
	
	return MojErrNone;
}

MojErr ImapBusDispatcher::AccountCreator::SmtpAccountCreatedResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ErrorToException(err);

		MojLogInfo(m_log, "created account successfully");

		m_message->replySuccess();

		Complete();
	} catch(const MojErrException& e) {
		MojLogError(m_log, "error creating account: %s", e.what());
		m_message->replyError(e.GetMojErr(), e.what());
		Failure(e);
	} catch(const exception& e) {
		MojLogError(m_log, "error creating account: %s", e.what());
		Failure(e);
	}

	return MojErrNone;
}

void ImapBusDispatcher::AccountCreator::Status(MojObject& status) const
{
	MojErr err;
	MojObject accountId;

	ImapBusDispatcherCommand::Status(status);

	err = m_payload.getRequired("accountId", accountId);
	ErrorToException(err);

	err = status.put("accountId", accountId);
	ErrorToException(err);
}

ImapBusDispatcher::ClientPtr ImapBusDispatcher::GetOrCreateClient(MojObject& accountId)
{
	// redo this
	MojLogTrace(m_log);

	ClientPtr client;
	ClientMap::iterator it;

	it = m_clients.find(accountId);
	if (it == m_clients.end()) {
		MojLogInfo(m_log, "creating client for account %s", AsJsonString(accountId).c_str());

		boost::shared_ptr<DatabaseInterface> dbInterface(new MojoDatabase(m_app.GetDbClient()));
		client.reset(new ImapClient(&(m_app.GetService()), this, accountId, dbInterface));

		m_clients[accountId] = client;
	} else {
		client = it->second;
	}

	assert(client.get());
	return client;
}

void ImapBusDispatcher::Status(MojObject& status) const
{
	MojErr err;
	MojObject clients(MojObject::TypeArray);

	ClientMap::const_iterator it;
	for(it = m_clients.begin(); it != m_clients.end(); ++it) {
		MojObject clientStatus;
		it->second->Status(clientStatus);
		err = clients.push(clientStatus);
		ErrorToException(err);
	}

	err = status.put("clients", clients);
	ErrorToException(err);

	if(!m_validators.empty()) {
		MojObject validators(MojObject::TypeArray);
		BOOST_FOREACH(const ValidatorPtr& validator, m_validators) {
			MojObject validatorStatus;
			validator->Status(validatorStatus);
			validators.push(validatorStatus);
		}

		err = status.put("validators", validators);
		ErrorToException(err);
	}

	if(m_commandManager->GetActiveCommandCount() > 0 || m_commandManager->GetPendingCommandCount() > 0) {
		MojObject commandManagerStatus;

		m_commandManager->Status(commandManagerStatus);

		err = status.put("busCommands", commandManagerStatus);
		ErrorToException(err);
	}

	if(m_networkStatusMonitor.get()) {
		MojObject monitorStatus;

#ifdef WEBOS_TARGET_MACHINE_STANDALONE
	//Set the Network Status to true and assumes the internet connection is available.
	MojObject cmStatus;
	err = cmStatus.put("isInternetConnectionAvailable", true);
	ErrorToException(err);
	m_networkStatusMonitor->SetFakeStatus(cmStatus, true /*persist set to true */);
#endif


		m_networkStatusMonitor->Status(monitorStatus);

		err = status.put("networkStatusMonitor", monitorStatus);
		ErrorToException(err);
	}
}

MojErr ImapBusDispatcher::StatusRequest(MojServiceMessage* msg, MojObject& payload)
{
	MojErr err;

	try {
		MojObject status;

		Status(status);

		bool subscribe = false;
		if(payload.get("subscribe", subscribe) && subscribe) {
			StatusSubscribe(msg);

			err = status.put("subscribed", true);
			ErrorToException(err);
		}

		msg->reply(status);
	} catch(const std::exception& e) {
		msg->replyError(MojErrInternal);
	}

	return MojErrNone;
}

void ImapBusDispatcher::StatusUpdate()
{
	MojObject status;

	Status(status);

	// Broadcast status
	BOOST_FOREACH(const MojRefCountedPtr<StatusSubscription>& sub, m_statusSubscriptions) {
		sub->GetServiceMessage()->reply(status);
	}
}

void ImapBusDispatcher::StatusSubscribe(MojServiceMessage* msg)
{
	MojRefCountedPtr<StatusSubscription> sub(new StatusSubscription(*this, msg));
	m_statusSubscriptions.push_back(sub);
}

void ImapBusDispatcher::StatusUnsubscribe(StatusSubscription* subscription)
{
	std::vector< MojRefCountedPtr<StatusSubscription> >::iterator it;

	for(it = m_statusSubscriptions.begin(); it != m_statusSubscriptions.end(); ++it) {
		if(it->get() == subscription) {
			m_statusSubscriptions.erase(it);
			break;
		}
	}
}

MojErr ImapBusDispatcher::SyncAccount(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {

		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId);

		SyncParams syncParams;
		syncParams.SetForce(true);

		ActivityPtr activity = GetActivity(payload);

		if(activity.get()) {
			syncParams.GetSyncActivities().push_back(activity);
		}

		if(activity.get() && !activity->GetName().empty()) {
			syncParams.SetReason("syncAccount with activity " + string(activity->GetName()));
		} else {
			syncParams.SetReason("syncAccount");
		}

		client->SyncAccount(syncParams);

		err = msg->replySuccess();
		ErrorToException(err);

		CheckActive();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

MojErr ImapBusDispatcher::SyncFolder(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		bool force = false;
		if(payload.contains("force")) {
			err = payload.getRequired("force", force);
			ErrorToException(err);
		}

		MojObject retry;
		MojString retryReason;

		if(payload.get("retry", retry)) {
			bool hasReason = false;
			err = retry.get("reason", retryReason, hasReason);
			ErrorToException(err);
		}

		if(!IsValidId(folderId))
			MojErrThrowMsg(MojErrInternal, "Invalid folder ID");

		ActivityPtr activity = GetActivity(payload);

		SyncParams syncParams;
		syncParams.SetPayload(payload);
		syncParams.SetForce(force);

		string reason = "syncFolder bus method called";

		if(!retryReason.empty()) {
			reason = "retry due to " + string(retryReason);
		} else if(force) {
			reason = "manual sync";
		} else if(activity.get()) {
			if(!activity->GetName().empty())
				reason = "syncFolder with activity " + string(activity->GetName());
			else {
				reason = "syncFolder on entering folder";

				// Debugging -- if "ignoreViewFolder" is enabled, we'll ignore the sync request.
				if(m_app.GetConfig().GetIgnoreViewFolder()) {
					err = msg->replySuccess();

					CheckActive();

					return MojErrNone;
				}
			}
		}

		syncParams.SetReason(reason);

		if(activity.get()) {
			syncParams.GetSyncActivities().push_back(activity);
		}

		client->SyncFolder(folderId, syncParams);

		err = msg->replySuccess();
		ErrorToException(err);

		CheckActive();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

// Upload changes to flags, deletions, etc
MojErr ImapBusDispatcher::SyncUpdate(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		ActivityPtr activity = ActivityParser::GetActivityFromPayload(payload);

		if(!IsValidId(folderId))
			MojErrThrowMsg(MojErrInternal, "Invalid folder ID");

		SyncParams syncParams;
		syncParams.SetReason("watch fired for changed emails");
		syncParams.SetSyncChanges(true);
		syncParams.SetSyncEmails(false);

		if(activity.get()) {
			syncParams.GetSyncActivities().push_back(activity);
		}

		client->SyncFolder(folderId, syncParams);

		err = msg->replySuccess();
		ErrorToException(err);

		CheckActive();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

MojErr ImapBusDispatcher::DownloadMessage(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		if(!IsValidId(folderId))
			MojErrThrowMsg(MojErrInternal, "Invalid folder ID");

		MojObject emailId;
		err = payload.getRequired("emailId", emailId);
		ErrorToException(err);

		// TODO deal with activity

		MojObject partId(MojObject::TypeUndefined);

		MojRefCountedPtr<DownloadListener> listener(new DownloadListener(msg, emailId, partId));
		client->DownloadPart(folderId, emailId, partId, listener);

		msg->replySuccess();

		CheckActive();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

MojErr ImapBusDispatcher::DownloadAttachment(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		if(!IsValidId(folderId))
			MojErrThrowMsg(MojErrInternal, "Invalid folder ID");

		// TODO deal with activity

		MojObject emailId;
		err = payload.getRequired("emailId", emailId);
		ErrorToException(err);

		MojObject partId;
		err = payload.getRequired("partId", partId);
		ErrorToException(err);

		MojRefCountedPtr<DownloadListener> listener(new DownloadListener(msg, emailId, partId));
		client->DownloadPart(folderId, emailId, partId, listener);

		msg->replySuccess();

		CheckActive();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

// Experimental
MojErr ImapBusDispatcher::SearchFolder(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(m_log);

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		if(!IsValidId(folderId))
			MojErrThrowMsg(MojErrInternal, "Invalid folder ID");

		// TODO deal with activity
		MojRefCountedPtr<SearchRequest> searchRequest(new SearchRequest());
		searchRequest->SetServiceMessage(msg);

		MojString searchText;
		err = payload.getRequired("searchText", searchText);
		ErrorToException(err);
		searchRequest->SetSearchText(std::string(searchText.data()));

		bool hasLimit = false;
		int limit = 0;
		err = payload.get("limit", limit, hasLimit);
		ErrorToException(err);

		if (hasLimit && limit > 0) {
			searchRequest->SetLimit(limit);
		}

		client->SearchFolder(folderId, searchRequest);

		CheckActive();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

ActivityPtr ImapBusDispatcher::GetActivity(MojObject& payload)
{
	return ActivityParser::GetActivityFromPayload(payload);
}

MojErr ImapBusDispatcher::WakeupIdle(MojServiceMessage* msg, MojObject& payload)
{
	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		client->WakeupIdle(folderId);

		CheckActive();

		msg->replySuccess();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

MojErr ImapBusDispatcher::StartIdle(MojServiceMessage* msg, MojObject& payload)
{
	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		SyncParams syncParams;
		syncParams.SetSyncEmails(false); // don't sync, just connect and idle
		syncParams.SetReason("IDLE setup");

		ActivityPtr activity = GetActivity(payload);
		if(activity.get()) {
			syncParams.GetConnectionActivities().push_back(activity);
		}

		client->SyncFolder(folderId, syncParams);

		msg->replySuccess();

		CheckActive();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

// Debug method
MojErr ImapBusDispatcher::MagicWand(MojServiceMessage* msg, MojObject& payload)
{
	try {
		MojObject accountId;
		vector<ClientPtr> clients;

		MojErr err;
		MojString command;
		err = payload.getRequired("command", command);
		ErrorToException(err);

		if(command == "reconfigure") {
			MojObject conf;

			err = payload.getRequired("config", conf);
			ErrorToException(err);

			err = m_app.configure(conf);
			ErrorToException(err);
		} else if(command == "abracadabra") {
			// easter egg
			static bool toggle = false;

			string temp = toggle ? "\x90\xc9\x89\x9e\x85\x85\x92\xc9\xd1\xcb\x9f\x99\x9e\x8e\x96"
					: "\x90\xc9\x83\x8a\x9f\xc9\xd1\xcb\xb0\xb6\xc7\xcb\xc9\x98\x87\x8e\x8e\x9d\x8e\x98\xc9\xd1\xcb\xb0\xb6\x96";

			toggle = !toggle;

			for(string::iterator it = temp.begin(); it != temp.end(); ++it) {
				*it ^= 0xEB;
			}

			MojObject payload;
			payload.fromJson(temp.data(), temp.length());

			msg->replySuccess(payload);
		} else if(command == "fakenet") {
			MojObject internet;

			bool persist = false;
			payload.get("persist", persist);

			if(payload.get("internet", internet)) {
				m_networkStatusMonitor->SetFakeStatus(internet, persist);
			} else if(payload.contains("preset")) {
				MojString preset;
				err = payload.getRequired("preset", preset);
				ErrorToException(err);

				MojObject internet;

				if(preset == "none") {
					err = internet.put("isInternetConnectionAvailable", false);
					ErrorToException(err);
				} else if(preset == "poor_wan") {
					MojObject wan;
					err = wan.putString("state", "connected");
					ErrorToException(err);
					err = wan.putString("ipAddress", "");
					ErrorToException(err);
					err = wan.put("isPersistent", true);
					ErrorToException(err);
					err = wan.putString("networkConfidenceLevel", "poor");
					ErrorToException(err);

					err = internet.put("isInternetConnectionAvailable", true);
					ErrorToException(err);

					err = internet.put("wan", wan);
					ErrorToException(err);
				} else {
					return msg->replyError(MojErrInternal, "invalid preset");
				}

				m_networkStatusMonitor->SetFakeStatus(internet, persist);
			} else {
				return msg->replyError(MojErrInternal, "need \"internet\" or \"preset\"");
			}

			return msg->replySuccess();
		} else {

			if(payload.get("accountId", accountId)) {
				ClientPtr client = GetOrCreateClient(accountId);
				client->MagicWand(msg, payload);
			} else if(!m_clients.empty()) {
				map<MojObject, ClientPtr>::const_iterator it;
				for(it = m_clients.begin(); it != m_clients.end(); ++it) {
					ClientPtr client = it->second;
					client->MagicWand(msg, payload);
				}
			} else {
				msg->replyError(MojErrInternal, "no clients");
			}
		}

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
}

bool ImapBusDispatcher::HasActiveClients()
{
	map<MojObject, ClientPtr>::const_iterator it;
	for(it = m_clients.begin(); it != m_clients.end(); ++it) {
		ClientPtr client = it->second;
		if(client->IsActive()) {
			return true;
		}
	}

	if(!m_validators.empty()) {
		return true;
	}

	if(m_commandManager->GetActiveCommandCount() > 0 || m_commandManager->GetPendingCommandCount() > 0) {
		return true;
	}

	return false;
}

void ImapBusDispatcher::UpdateClientActive(ImapClient* client, bool active)
{
	CheckActive();

	// Update status subscriptions
	StatusUpdate();
}

void ImapBusDispatcher::CheckActive()
{
	bool hadTimeout = false;

	// Remove any existing timeout
	if(m_inactivityCallbackId > 0) {
		g_source_remove(m_inactivityCallbackId);
		m_inactivityCallbackId = 0;
		hadTimeout = true;
	}

	ImapConfig& imapConfig = m_app.GetConfig();
	int timeoutSeconds = imapConfig.GetInactivityTimeout();

	if(timeoutSeconds > 0) {
		// If there's no active clients, schedule shutdown timeout
		if(!HasActiveClients()) {

			m_inactivityCallbackId = g_timeout_add_seconds(timeoutSeconds, &ImapBusDispatcher::InactivityCallback, gpointer(this));

			MojLogDebug(m_log, "no clients active; %s shutdown in %d seconds id=%d", hadTimeout ? "rescheduling" : "scheduling", timeoutSeconds, m_inactivityCallbackId);

		} else if(hadTimeout) {
			MojLogDebug(m_log, "at least one client is now active; cancelling shutdown timeout");
		}
	}
}

gboolean ImapBusDispatcher::InactivityCallback(gpointer data)
{
	ImapBusDispatcher* dispatcher = reinterpret_cast<ImapBusDispatcher*>(data);

	dispatcher->m_inactivityCallbackId = 0;

	if(!dispatcher->HasActiveClients()) {
		MojLogNotice(dispatcher->m_log, "shutting down; inactivity timer expired");
		dispatcher->m_app.shutdown();
	} else {
		MojLogInfo(dispatcher->m_log, "not shutting down; at least one client is still active");
	}

	return false;
}

void ImapBusDispatcher::CommandComplete(Command* command)
{
	m_commandManager->CommandComplete(command);

	CheckActive();
}
