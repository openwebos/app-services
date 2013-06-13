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

#include "SmtpCommon.h"
#include "SmtpBusDispatcher.h"
#include "core/MojServiceMessage.h"
#include "commands/SaveEmailCommand.h"
#include "SmtpServiceApp.h"
#include "SmtpClient.h"
#include "data/DatabaseInterface.h"
#include "data/MojoDatabase.h"
#include "data/SmtpAccountAdapter.h"
#include "data/FolderAdapter.h"
#include "activity/ActivityBuilder.h"
#include "data/EmailSchema.h"
#include "data/DatabaseAdapter.h"
#include "activity/ActivityParser.h"
#include "exceptions/MojErrException.h"
#include "SmtpConfig.h"

MojLogger SmtpBusDispatcher::s_log("com.palm.smtp");

SmtpBusDispatcher::SmtpBusDispatcher(SmtpServiceApp& app)
: m_app(app),
  m_dbClient(app.GetDbClient()),
  m_dbInterface(app.GetDbClient()),
  m_client(&(app.GetService())),
  m_tasks(new CommandManager(1000)),
  m_shutdownId(0)
{
}

SmtpBusDispatcher::~SmtpBusDispatcher()
{
}

MojErr SmtpBusDispatcher::InitHandler()
{
	RegisterMethods();
	return MojErrNone;
}

MojErr SmtpBusDispatcher::RegisterMethods()
{
	MojErr err = addMethod("ping", (Callback) &SmtpBusDispatcher::Ping);
	MojErrCheck(err);

	err = addMethod("status", (Callback) &SmtpBusDispatcher::Status);
	MojErrCheck(err);

	err = addMethod("validateAccount", (Callback) &SmtpBusDispatcher::ValidateAccount);
	MojErrCheck(err);
	
	err = addMethod("accountCreated", (Callback) &SmtpBusDispatcher::AccountCreated);
	MojErrCheck(err);

	err = addMethod("accountEnabled", (Callback) &SmtpBusDispatcher::AccountEnabled);
	MojErrCheck(err);

	err = addMethod("accountUpdated", (Callback) &SmtpBusDispatcher::AccountUpdated);
	MojErrCheck(err);

	err = addMethod("credentialsChanged", (Callback) &SmtpBusDispatcher::AccountUpdated);
	MojErrCheck(err);

	err = addMethod("accountDeleted", (Callback) &SmtpBusDispatcher::AccountDeleted);
	MojErrCheck(err);

	err = addMethod("simpleSendEmail", (Callback) &SmtpBusDispatcher::SimpleSendEmail);
	MojErrCheck(err);

	err = addMethod("sendMail", (Callback) &SmtpBusDispatcher::SendMail);
	MojErrCheck(err);

	err = addMethod("saveDraft", (Callback) &SmtpBusDispatcher::SaveDraft);
	MojErrCheck(err);

	err = addMethod("syncOutbox", (Callback) &SmtpBusDispatcher::SyncOutbox);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr SmtpBusDispatcher::Ping(MojServiceMessage* msg, MojObject& payload)
{
	MojErr err;
	
	err = msg->replySuccess();
	MojErrCheck(err);
	
	return MojErrNone;
}

MojErr SmtpBusDispatcher::Status(MojServiceMessage* msg, MojObject& payload)
{
	MojErr err;
	MojObject status;

	try {
		if(m_tasks->GetActiveCommandCount() > 0 || m_tasks->GetActiveCommandCount()) {
			MojObject tasksStatus;
			err = tasksStatus.putBool(MojServiceMessage::ReturnValueKey, true);

			m_tasks->Status(tasksStatus);
			err = status.put("tasks", tasksStatus);
			ErrorToException(err);
		}

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

	} catch(const std::exception& e) {
		msg->replyError(MojErrInternal, e.what());
	} catch(...) {
		msg->replyError(MojErrInternal);
	}
	
	err = msg->reply(status);
	MojErrCheck(err);
	
	return MojErrNone;
}

/*
 * Writes a message to a folder (typically outbox or drafts)
 * 
 * Params: {"accountId": accountId, "draft": true/false, "email": {...}}
 * 
 * email should be a standard email object with the following modifications:
 * - "_kind" will be set to the current com.palm.email kind (TODO)
 * - "folderId" will be set to the appropriate outbox or drafts folder
 * - parts may provide a "content" field, which will be written to the file cache and replaced with a "path"
 */
MojErr SmtpBusDispatcher::SendMail(MojServiceMessage* msg, MojObject& payload)
{
	CancelShutdown();

	MojErr err;
	
	MojObject email;
	err = payload.getRequired("email", email);
	MojErrCheck(err);
	
	MojObject accountId;
	err = payload.getRequired("accountId", accountId);
	MojErrCheck(err);
	
	bool isDraft = false;
	if(payload.contains("draft")) {
		err = payload.getRequired("draft", isDraft);
		MojErrCheck(err);
	}
	
	/*
	CommandClient &commandClient = m_app.GetOfflineCommandClient();
	
	// MojServiceMessage is internally refcounted, so wrapping it with MojRefCountedPtr
	// will share the refcount.
	MojRefCountedPtr<MojServiceMessage> msgPtr(msg);
	
	MojRefCountedPtr<SaveEmailCommand> command;
	command.reset( new SaveEmailCommand(commandClient, m_app.GetDbClient(), m_app.GetFileCacheClient(), msgPtr, email, accountId, isDraft));
	commandClient.RunCommand(command);
	*/

	MojRefCountedPtr<MojServiceMessage> msgPtr(msg);
	ClientPtr client = GetOrCreateClient(accountId);
	client->SaveEmail(msgPtr, email, accountId, isDraft);
	
	return MojErrNone;
}

MojErr SmtpBusDispatcher::SaveDraft(MojServiceMessage* msg, MojObject& payload)
{
	CancelShutdown();

	MojErr err;

	MojObject email;
	err = payload.getRequired("email", email);
	MojErrCheck(err);

	MojObject accountId;
	err = payload.getRequired("accountId", accountId);
	MojErrCheck(err);

	// last parameter to SaveEmail indicates whether this email is a draft
	MojRefCountedPtr<MojServiceMessage> msgPtr(msg);
	ClientPtr client = GetOrCreateClient(accountId);
	client->SaveEmail(msgPtr, email, accountId, true);

	return MojErrNone;
}

SmtpBusDispatcher::ClientPtr SmtpBusDispatcher::GetOrCreateClient(MojObject& accountId)
{
	MojLogTrace(s_log);

	ClientPtr client;
	ClientMap::iterator it;

	it = m_clients.find(accountId);
	if (it == m_clients.end()) {
		MojLogInfo(s_log, "Creating SmtpClient");
		client = CreateClient();
		m_clients[accountId] = client;

		MojLogInfo(s_log, "Setting smtp client with account id");
		client->SetAccountId(accountId);
	} else {
		client = it->second;
	}

	assert(client.get());
	return client;
}

SmtpBusDispatcher::ClientPtr SmtpBusDispatcher::CreateClient()
{
	MojLogTrace(s_log);

	boost::shared_ptr<DatabaseInterface> databaseInterface(new MojoDatabase(m_app.GetDbClient()));
	boost::shared_ptr<DatabaseInterface> tempDatabaseInterface(new MojoDatabase(m_app.GetTempDbClient()));
	MojRefCountedPtr<SmtpClient> client(new SmtpClient(this, databaseInterface, tempDatabaseInterface, &(m_app.GetService())));

	return client;
}

MojErr SmtpBusDispatcher::ValidateAccount(MojServiceMessage* msg, MojObject& payload)
{

	CancelShutdown();

	try {
		MojRefCountedPtr<Validator> validator(new Validator(*this, m_dbInterface, msg, payload));
		validator->Start();
	} catch (const std::exception& e) {
		// TODO
	}

	return MojErrNone;
}

void SmtpBusDispatcher::ValidateSmtpAccount(boost::shared_ptr<SmtpAccount> account, MojRefCountedPtr<MojServiceMessage> msg, MojObject& payload)
{
	MojLogInfo(s_log, "SmtpBusDispatcher::ValidateSmtpAccount");

	new SmtpValidator(this, m_tasks, account, &(m_app.GetService()), msg.get(), payload);
}

MojErr SmtpBusDispatcher::AccountCreated(MojServiceMessage* msg, MojObject& payload)
{
	CancelShutdown();

	MojLogNotice(s_log, "creating SMTP account config");

	try {
		MojRefCountedPtr<AccountCreator> creator (new AccountCreator(*this, msg, payload));
		creator->CreateSmtpAccount();
	} catch (const std::exception& e) {
		msg->replyError(MojErrNone); // TODO
	}

	return MojErrNone;
}

MojErr SmtpBusDispatcher::AccountEnabled(MojServiceMessage* msg, MojObject& payload)
{
	CancelShutdown();

	MojLogTrace(s_log);

	MojLogInfo(s_log, "SmtpBusDispatcher::AccountEnabled");

	MojString json;
	payload.toJson(json);
	MojLogInfo(s_log, "AccountEnabled with payload %s", json.data());
	
	MojObject accountId;
	MojErr err = payload.getRequired("accountId", accountId);
	MojErrCheck(err);

	bool enabled = true;
	payload.get("enabled", enabled);

	bool force = true;
	payload.get("force", force);

	MojLogInfo(s_log, "AccountEnabled got enabled %d", enabled ? 1: 0);

	ClientPtr client = GetOrCreateClient(accountId);

	MojLogInfo(s_log, "AccountEnabled got ClientPtr %p", client.get());
	
	if (enabled) {
		// Setup account and outbox watches
		client->EnableAccount(msg);
	} else {
		// Terminate any activity, and remove account watches
		client->DisableAccount(msg);
	}
	
	return MojErrNone;
}

MojErr SmtpBusDispatcher::AccountUpdated(MojServiceMessage* msg, MojObject& payload)
{
	CancelShutdown();

	MojString json;
	MojErr err = payload.toJson(json);
	MojErrCheck(err);
	MojLogInfo(s_log, "SmtpBusDispatcher::AccountUpdated: payload=%s", json.data());

	MojObject accountId;
	err = payload.getRequired("accountId", accountId);
	MojErrCheck(err);

	try {
		ClientPtr client = GetOrCreateClient(accountId);
		MojRefCountedPtr<AccountUpdater> accountUpdater(new AccountUpdater(*this, client, msg, accountId, payload));
		accountUpdater->UpdateAccount();
	} catch(const MojErrException& e) {
		MojLogError(s_log, "error in %s: %s", __PRETTY_FUNCTION__, e.what());
		msg->replyError(e.GetMojErr());
	} catch(const exception& e) {
		MojLogError(s_log, "error in %s: %s", __PRETTY_FUNCTION__, e.what());
		msg->replyError(MojErrInternal, e.what());
	} catch(...) {
		msg->replyError(MojErrInternal);
	}

	return MojErrNone;
}

MojErr SmtpBusDispatcher::AccountDeleted(MojServiceMessage* msg, MojObject& payload)
{
	CancelShutdown();

	MojLogTrace(s_log);

	MojObject accountId;
	MojErr err = payload.getRequired("accountId", accountId);
	MojErrCheck(err);

	ClientPtr client = GetOrCreateClient(accountId);

	MojLogInfo(s_log, "AccountDeleted got ClientPtr %p", client.get());

	MojLogInfo(s_log, "Disabling account");

	// Terminate any activity, and remove account watches
	client->DisableAccount(msg);

	return MojErrNone;
}

MojErr SmtpBusDispatcher::SimpleSendEmail(MojServiceMessage* msg, MojObject& payload)
{
	CancelShutdown();

	// Setup the account
	boost::shared_ptr<SmtpAccount> account(new SmtpAccount());

	MojString hostname;
	MojErr err = payload.getRequired("server", hostname);
	MojErrCheck(err);
	account->SetHostname(hostname.data());

	int port;
	err = payload.getRequired("port", port);
	MojErrCheck(err);
	account->SetPort(port);

	MojString username;
	err = payload.getRequired("username", username);
	MojErrCheck(err);
	account->SetUsername(username.data());

	MojString password;
	err = payload.getRequired("password", password);
	MojErrCheck(err);
	account->SetPassword(password.data());

	MojString email;
	err = payload.getRequired("email", email);
	MojErrCheck(err);
	account->SetEmail(email.data());
	
	MojString encryptionStr;
	err = payload.getRequired("encryption", encryptionStr);
	MojErrCheck(err);
	
	if (encryptionStr == "none")
		account->SetEncryption(SmtpAccount::Encrypt_None);
	else if(encryptionStr == "ssl")
		account->SetEncryption(SmtpAccount::Encrypt_SSL);
	else if(encryptionStr == "tls")
		account->SetEncryption(SmtpAccount::Encrypt_TLS);
	else if(encryptionStr == "tlsIfAvailable")
		account->SetEncryption(SmtpAccount::Encrypt_TLSIfAvailable);
	else {
		// Todo: Make Real Error
		ErrorToException(MojErrInvalidArg);
	}

	MojString fromAddress;
	err = payload.getRequired("from", fromAddress);
	MojErrCheck(err);

	MojString toAddress; // FIXME: Support multiple addresses
	err = payload.getRequired("to", toAddress);
	MojErrCheck(err);

	MojString mailData;
	err = payload.getRequired("data", mailData);
	MojErrCheck(err);
	
	new SmtpSimpleSender(m_tasks, account, &(m_app.GetService()), msg, fromAddress, toAddress, mailData);

	return MojErrNone;

}

MojErr SmtpBusDispatcher::SyncOutbox(MojServiceMessage* msg, MojObject& payload)
{
	CancelShutdown();

	MojString json;
	payload.toJson(json);
	MojLogInfo(s_log, "SyncOutbox with payload %s", json.data());
	
	MojObject accountId;
	MojErr err = payload.getRequired("accountId", accountId);
	MojErrCheck(err);

	bool force = false;
	payload.get("force",  force);
	MojErrCheck(err);

	MojObject folderId;
	payload.get("folderId", folderId);
	MojErrCheck(err);

	ClientPtr client = GetOrCreateClient(accountId);
	
	try {
		MojRefCountedPtr<OutboxSyncer> outboxSyncer(new OutboxSyncer(*this, client, msg, accountId, folderId, force, payload));
		outboxSyncer->SyncOutbox();
		
	} catch (const std::exception& e) {
		msg->replyError(MojErrNone);
	}

	return MojErrNone;
}

void SmtpBusDispatcher::PrepareShutdown()
{
	int inactivityTimeout = SmtpConfig::GetConfig().GetInactivityTimeout();

	CancelShutdown();

	if(inactivityTimeout > 0) {
		MojLogTrace(s_log);
		MojLogInfo(s_log, "Checking SMTP clients to see if SMTP service can be shutdown");

		bool shutdown = true;
		ClientMap::iterator it;
		for (it = m_clients.begin(); it != m_clients.end(); ++it) {
			ClientPtr client = (*it).second;
			if (client->IsActive()) {
				shutdown = false;
				break;
			}
		}

		if (shutdown) {
			MojLogInfo(s_log, "Setting up SMTP shutdown callback");
			m_shutdownId = g_timeout_add_seconds(inactivityTimeout, &SmtpBusDispatcher::ShutdownCallback, this);
		}
	}
}

void SmtpBusDispatcher::CancelShutdown()
{
	if (m_shutdownId > 0) {
		g_source_remove(m_shutdownId);
		MojLogInfo(s_log, "SMTP service shutdown attempt %d cancelled", m_shutdownId);
		m_shutdownId = 0;
	}
}

gboolean SmtpBusDispatcher::ShutdownCallback(void* data)
{
	if (data == NULL) {
		MojLogError(s_log, "SmtpBusDispatcher::ShutdownCallback did not receive instance of SmtpBusDispatcher");
		return false;
	}

	try {
		MojLogInfo(s_log, "SMTP service is shutting down");
		SmtpBusDispatcher* smtpBusDispatcher = static_cast<SmtpBusDispatcher*>(data);
		smtpBusDispatcher->Shutdown();
	} catch (const std::exception& ex) {
		MojLogError(s_log, "Failed to shut down SMTP service: %s", ex.what());
	} catch (...) {
		MojLogError(s_log, "Unknown exception in shutting down SMTP service");
	}

	return false;
}

void SmtpBusDispatcher::Shutdown()
{
	m_app.shutdown();
	MojLogInfo(s_log, "SMTP service shutdown");
}

SmtpBusDispatcher::AccountCreator::AccountCreator(SmtpBusDispatcher& dispatcher,
												  MojServiceMessage* msg,
												  MojObject& payload)
: m_log(SmtpBusDispatcher::s_log),
  m_dispatcher(dispatcher),
  m_msg(msg),
  m_payload(payload),
  m_mergeSlot(this, &SmtpBusDispatcher::AccountCreator::MergeSmtpConfigResponse)
{

}

SmtpBusDispatcher::AccountCreator::~AccountCreator()
{
}

void SmtpBusDispatcher::AccountCreator::CreateSmtpAccount() 
{
	/* Payload:
	 *  {"accountId":"2+Cb",
	 *   "config":{"domain":"Gmail","email":"foo@gmail.com","encryption":"ssl","port":993,
	 *             "server":"imap.gmail.com",
	 *             "smtpConfig":{"email":"foo@gmail.com","encryption":"ssl","port":465,
	 *                           "server":"smtp.gmail.com","username":"foo@gmail.com"},
	 *              "username":"foo@gmail.com"},     
	 *   "mailAccountId":"2+Ce"}
	 */

	MojObject mailAccountId;
	MojErr err = m_payload.getRequired("mailAccountId", mailAccountId);
	ErrorToException(err);

	MojObject payloadConfig;
	err = m_payload.getRequired("config", payloadConfig);
	ErrorToException(err);
	MojObject payloadSmtpConfig;
	err = payloadConfig.getRequired("smtpConfig", payloadSmtpConfig);
	ErrorToException(err);

	MojObject smtpConfig;
	SmtpAccountAdapter::GetSmtpConfigFromPayload(payloadSmtpConfig, smtpConfig);

	MojObject smtpConfigMergeObject;
	err = smtpConfigMergeObject.put("_id", mailAccountId);
	ErrorToException(err);
	err = smtpConfigMergeObject.put("smtpConfig", smtpConfig);
	ErrorToException(err);

	err = m_dispatcher.m_dbClient.merge(m_mergeSlot, smtpConfigMergeObject);

	ErrorToException(err);
}

MojErr SmtpBusDispatcher::AccountCreator::MergeSmtpConfigResponse(MojObject& response, MojErr err)
{
	m_dispatcher.PrepareShutdown();

	try {
		ErrorToException(err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		if(results.size() != 1) {
			throw MailException("no merge results", __FILE__, __LINE__);
		}

		MojLogNotice(m_log, "created SMTP config successfully");

	} catch(const std::exception& e) {
		MojLogError(m_log, "error merging SMTP config into account: %s", e.what());
		m_msg->replyError(MojErrInternal, e.what());
	}

	return m_msg->replySuccess();
}

SmtpBusDispatcher::AccountUpdater::AccountUpdater(SmtpBusDispatcher& dispatcher,
														  ClientPtr smtpClient,
														  MojServiceMessage* msg,
														  MojObject& accountId,
														  MojObject& payload)
: m_log(SmtpBusDispatcher::s_log),
  m_dispatcher(dispatcher),
  m_accountClient(smtpClient),
  m_msg(msg),
  m_accountId(accountId),
  m_payload(payload),
  m_activityUpdatedSlot(this, &SmtpBusDispatcher::AccountUpdater::ActivityUpdated),
  m_activityErrorSlot(this, &SmtpBusDispatcher::AccountUpdater::ActivityError)
{

}

SmtpBusDispatcher::AccountUpdater::~AccountUpdater()
{

}

// TODO: switch to ActivitySet and use factory to create activity
void SmtpBusDispatcher::AccountUpdater::UpdateAccount()
{
	MojErr err = m_payload.getRequired("accountId", m_accountId);
	ErrorToException(err);

	ActivityParser ap;
	ap.ParseCallbackPayload(m_payload);

	MojString json;
	m_payload.toJson(json);
	MojLogInfo(s_log, "ActivityPayload: %s", json.data());

	m_activity = ActivityParser::GetActivityFromPayload(m_payload);
	ap.GetActivityId().stringValue(m_activityId);
	m_activityName = ap.GetActivityName();

	if ( m_activity.get() ) {
		MojLogInfo(s_log, "AccountUpdater adopting activity");
		m_activity->SetSlots(m_activityUpdatedSlot, m_activityErrorSlot);
		m_activity->Adopt(m_dispatcher.m_client);
	} else {
		MojLogInfo(s_log, "AccountUpdater creating activity");
		// If an activity wasn't provided, create a new one just for this purpose
		ActivityBuilder ab;
		MojString name;
		MojErr err = name.format("SMTP Internal AccountUpdate Activity for account %s", AsJsonString(m_accountId).c_str());
		ErrorToException(err);
		ab.SetName(name);
		ab.SetDescription("Activity representing SMTP Account updater");
		ab.SetImmediate(true, ActivityBuilder::PRIORITY_LOW);
		m_activity = Activity::PrepareNewActivity(ab);
		m_activity->SetSlots(m_activityUpdatedSlot, m_activityErrorSlot);
		m_activity->Create(m_dispatcher.m_client);
		m_activityName.assign( name );
	}
}

MojErr SmtpBusDispatcher::AccountUpdater::ActivityUpdated(Activity * activity, Activity::EventType e)
{
	MojLogInfo(s_log, "AccountUpdater has updated activity, activityId=%s activityName=%s", m_activityId.data(), m_activityName.data());

	if (e == Activity::StartEvent) {
		MojLogInfo(s_log, "AccountUpdater calling client to update account");

		m_activityUpdatedSlot.cancel();
		m_activityErrorSlot.cancel();

		m_accountClient->UpdateAccount(m_activity, m_activityId, m_activityName, m_accountId);

		m_msg->replySuccess();
	}

	return MojErrNone;
}

MojErr SmtpBusDispatcher::AccountUpdater::ActivityError(Activity * activity, Activity::ErrorType e, const std::exception& exc)
{
	MojLogInfo(s_log, "Outboxsyncer has activity error, activity %p, errType=%d, exc->what=%s", activity, e, exc.what());

	m_activityUpdatedSlot.cancel();
	m_activityErrorSlot.cancel();

	m_msg->replyError(MojErrInternal);

	return MojErrNone;
}

SmtpBusDispatcher::OutboxSyncer::OutboxSyncer(SmtpBusDispatcher& dispatcher,
														  ClientPtr smtpClient,
														  MojServiceMessage* msg,
														  MojObject& accountId,
														  MojObject& folderId,
														  bool force,
														  MojObject& payload)
: m_log(SmtpBusDispatcher::s_log),
  m_dispatcher(dispatcher),
  m_accountClient(smtpClient),
  m_msg(msg),
  m_accountId(accountId),
  m_folderId(folderId),
  m_force(force),
  m_payload(payload),
  m_activityUpdatedSlot(this, &SmtpBusDispatcher::OutboxSyncer::ActivityUpdated),
  m_activityErrorSlot(this, &SmtpBusDispatcher::OutboxSyncer::ActivityError)
{

}

SmtpBusDispatcher::OutboxSyncer::~OutboxSyncer()
{

}

// TODO: switch to ActivitySet and use factory to create activity
void SmtpBusDispatcher::OutboxSyncer::SyncOutbox()
{
	MojErr err = m_payload.getRequired("accountId", m_accountId);
	ErrorToException(err);

	ActivityParser ap;
	ap.ParseCallbackPayload(m_payload);
	
	MojString json;
	m_payload.toJson(json);
	MojLogInfo(s_log, "ActivityPayload: %s", json.data());

	m_activity = ActivityParser::GetActivityFromPayload(m_payload);
	ap.GetActivityId().stringValue(m_activityId);
	m_activityName = ap.GetActivityName();;
	
	if ( m_activity.get() ) {
		MojLogInfo(s_log, "OutboxSyncer adopting activity");
		m_activity->SetSlots(m_activityUpdatedSlot, m_activityErrorSlot);
		m_activity->Adopt(m_dispatcher.m_client);
	} else {
		MojLogInfo(s_log, "OutboxSyncer creating activity");
		// If an activity wasn't provided, create a new one just for this purpose
		ActivityBuilder ab;
		MojString name;
		MojErr err = name.format("SMTP Internal Outbox Sync Activity for account %s", AsJsonString(m_accountId).c_str());
		ErrorToException(err);
		ab.SetName(name);
		ab.SetDescription("Activity representing SMTP Outbox Sync");
		ab.SetImmediate(true, ActivityBuilder::PRIORITY_LOW);
		m_activity = Activity::PrepareNewActivity(ab);
		m_activity->SetSlots(m_activityUpdatedSlot, m_activityErrorSlot);
		m_activity->Create(m_dispatcher.m_client);
		m_activityName.assign( name );
	}
}

MojErr SmtpBusDispatcher::OutboxSyncer::ActivityUpdated(Activity * activity, Activity::EventType event)
{
	MojLogInfo(s_log, "Outboxsyncer has updated activity, activityId=%s activityName=%s event=%d", m_activityId.data(), m_activityName.data(), event);
	
	switch(event) {
	case Activity::StartEvent:
		MojLogInfo(s_log, "OutboxSyncer calling client to sync outbox");
		
		m_activityUpdatedSlot.cancel();
		m_activityErrorSlot.cancel();

		m_accountClient->SyncOutbox(m_activity, m_activityId, m_activityName, m_accountId, m_folderId, m_force);
		
		m_msg->replySuccess();
		break;
	default:
		// ignore other states
		break;
	}
	
	return MojErrNone;
}

MojErr SmtpBusDispatcher::OutboxSyncer::ActivityError(Activity * activity, Activity::ErrorType e, const std::exception& exc)
{
	MojLogInfo(s_log, "Outboxsyncer has activity error, activity %p, errType=%d, exc->what=%s", activity, e, exc.what());

	m_activityUpdatedSlot.cancel();
	m_activityErrorSlot.cancel();

	m_msg->replyError(MojErrInternal);
	
	return MojErrNone;
}

SmtpBusDispatcher::Validator::Validator(SmtpBusDispatcher& dispatcher,
							 		    MojoDatabase& dbInterface,
										MojServiceMessage* msg,
										MojObject& payload)
: m_log("com.palm.pop"),
  m_dispatcher(dispatcher),
  m_dbInterface(dbInterface),
  m_msg(msg),
  m_payload(payload),
  m_findMailAccountSlot(this, &SmtpBusDispatcher::Validator::FindMailAccountResponse)
{

}

void SmtpBusDispatcher::Validator::Start()
{
	MojObject accountId;

	if(m_payload.get(SmtpAccountAdapter::ACCOUNT_ID, accountId)) {
		MojLogInfo(s_log, "Getting accountId from DB");
		m_dbInterface.GetAccount(m_findMailAccountSlot, accountId);
	} else {

		MojObject nothing;
		Validate(nothing);
	}

}

MojErr SmtpBusDispatcher::Validator::FindMailAccountResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojObject accountObj;
		DatabaseAdapter::GetOneResult(response, accountObj);
		Validate(accountObj);

	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;
}

void SmtpBusDispatcher::Validator::Validate(MojObject& accountObj) {
	MojLogInfo(s_log, "Validate");
	try {
		boost::shared_ptr<SmtpAccount> account(new SmtpAccount());
		SmtpAccountAdapter::GetAccountFromValidationPayload(m_payload, accountObj, account);
		m_dispatcher.ValidateSmtpAccount(account, m_msg, m_payload);
	} catch (const std::exception& e) {
		MojLogError(s_log, "%s", e.what());
	}
}
