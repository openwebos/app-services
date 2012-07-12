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

#include "PopDefs.h"
#include "PopMain.h"
#include "PopBusDispatcher.h"
#include "client/DownloadListener.h"
#include "core/MojErr.h"
#include "data/MojoDatabase.h"
#include "data/PopAccount.h"
#include "data/PopAccountAdapter.h"
#include "luna/MojLunaMessage.h"
#include "network/SocketConnection.h"
#include "PopValidator.h"
#include "ParseEmlHandler.h"
#include <iostream>
#include "data/DatabaseAdapter.h"

#include "client/FileCacheClient.h"

using namespace boost;
using namespace std;

// set up the method callbacks
const PopBusDispatcher::Method PopBusDispatcher::s_methods[] = {
	{_T("parseEmlMessage"), (Callback) &PopBusDispatcher::ParseEmlMessage},
	{_T("validateAccount"), (Callback) &PopBusDispatcher::ValidateAccount},
	{_T("accountCreated"), (Callback) &PopBusDispatcher::AccountCreated},
	{_T("accountEnabled"), (Callback) &PopBusDispatcher::AccountEnabled},
	{_T("accountUpdated"), (Callback) &PopBusDispatcher::AccountUpdated},
	{_T("accountDeleted"), (Callback) &PopBusDispatcher::AccountDeleted},
	{_T("credentialsChanged"), (Callback) &PopBusDispatcher::CredentialsChanged},
	{_T("syncFolder"), (Callback) &PopBusDispatcher::SyncFolder},
	{_T("syncAccount"), (Callback) &PopBusDispatcher::SyncAccount},
	{_T("moveToSentFolder"), (Callback) &PopBusDispatcher::MoveToSentFolder},
	{_T("moveEmail"), (Callback) &PopBusDispatcher::MoveEmail},
	{_T("downloadMessage"), (Callback) &PopBusDispatcher::DownloadMessage},
	{_T("downloadAttachment"), (Callback) &PopBusDispatcher::DownloadAttachment},
	{_T("ping"), (Callback) &PopBusDispatcher::Ping},
	{_T("status"), (Callback) &PopBusDispatcher::Status},
	{NULL, NULL} };

// MojoDB database kind
const char* const PopBusDispatcher::POP_KIND = _T("popKind:1");

MojLogger PopBusDispatcher::s_log(_T("com.palm.pop"));

// constructor registers the putKind callback in its signal slot
PopBusDispatcher::PopBusDispatcher(PopMain& app, MojDbServiceClient& dbClient,
								   MojLunaService& service)
: m_app(app),
  m_tasks(new CommandManager(1000)),
  m_dbClient(dbClient),
  m_dbInterface(dbClient),
  m_service(service),
  m_bClient(new BusClient(&service)),
  m_shutdownId(0),
  m_putKindSlot(this, &PopBusDispatcher::PutKindResult)
{

	MojLogTrace(s_log);
}

PopBusDispatcher::~PopBusDispatcher()
{
	MojLogTrace(s_log);
}

// This method is called once per session
MojErr PopBusDispatcher::Init()
{
	MojLogTrace(s_log);

	// Add methods available on the bus
	MojErr err = addMethods(s_methods);
	ErrorToException(err);

	return MojErrNone;
}

// callback for MojoDB putKind
MojErr PopBusDispatcher::PutKindResult(MojObject& result, MojErr err)
{
	MojLogTrace(s_log);

	if (err)
		MojLogDebug(s_log, _T("database putKind FAILED."));
	else
		MojLogDebug(s_log, _T("database putKind success."));

	return MojErrNone;
}

MojErr PopBusDispatcher::ValidateAccount(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);

	boost::shared_ptr<PopAccount> account(new PopAccount());

	try {
		MojObject accountId;
		if(payload.get("accountId", accountId)) {
			MojRefCountedPtr<FirstUseValidator> firstUseValidator(new FirstUseValidator(*this, m_dbInterface, accountId, msg, payload));
			firstUseValidator->Validate();
		} else {
			// NOT First Use
			boost::shared_ptr<PopAccount> account(new PopAccount());
			GetAccountFromPayload(account, payload);
			ValidatePopAccount(account, msg, payload);
		}
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;
}

void PopBusDispatcher::ValidatePopAccount(boost::shared_ptr<PopAccount> account, MojRefCountedPtr<MojServiceMessage> msg, MojObject& payload)
{
	MojLogInfo(s_log, "PopBusDispatcher::ValidatePopAccount");

	new PopValidator(this, m_tasks, account, msg.get(), payload);
}

MojErr PopBusDispatcher::ParseEmlMessage(MojServiceMessage* msg, MojObject& payload)
{
	MojLogInfo(s_log, "PopBusDispatcher::ParseEmlMessage");

	// cancel shut down if it is in shut down state
	CancelShutdown();

	try {

		MojString filePath;
		MojErr err = payload.getRequired("filePath", filePath);
		ErrorToException(err);

		MojLogDebug(s_log, "filePath:%s", filePath.data());
		if (filePath.empty()) {
			msg->replyError(MojErrInvalidArg, "Invalid query string");
		} else {
			new ParseEmlHandler(this, m_bClient, m_tasks, msg, filePath.data());
		}

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

}

MojErr PopBusDispatcher::AccountCreated(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);
	
		MojRefCountedPtr<AccountCreator> creator(new AccountCreator(*this, msg, payload));
		creator->CreatePopAccount();

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;
}

MojErr PopBusDispatcher::AccountEnabled(MojServiceMessage* msg, MojObject& payload)
{
	// cacel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);
	MojLogInfo(s_log, "AccountEnabled's payload: %s", AsJsonString(payload).c_str());

	bool enabled = false;
	MojErr err = payload.getRequired("enabled", enabled);
	ErrorToException(err);

	MojObject accountId;
	err = payload.getRequired("accountId", accountId);
	ErrorToException(err);

	if(enabled) {
		ClientPtr client = GetOrCreateClient(accountId, false);
		client->EnableAccount(msg, payload);
		client->UpdateAccount(payload, false);
	}
	else {
		ClientPtr client = GetOrCreateClient(accountId, false);
		client->DisableAccount(msg, payload);
	}

	return MojErrNone;
}

MojErr PopBusDispatcher::AccountUpdated(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);

	MojString json;
	MojErr err = payload.toJson(json);
	ErrorToException(err);
	MojLogInfo(s_log, "PopBusDispatcher::AccountUpdated: payload=%s", json.data());

	MojObject accountId;
	err = payload.getRequired("accountId", accountId);
	ErrorToException(err);

	try {
		ClientPtr client = GetOrCreateClient(accountId);
		client->UpdateAccount(payload, false);
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return msg->replySuccess();
}

MojErr PopBusDispatcher::CredentialsChanged(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);

	MojString json;
	MojErr err = payload.toJson(json);
	ErrorToException(err);
	MojLogInfo(s_log, "PopBusDispatcher::CredentialsChanged: payload=%s", json.data());

	MojObject accountId;
	err = payload.getRequired("accountId", accountId);
	ErrorToException(err);

	try {
		ClientPtr client = GetOrCreateClient(accountId);
		// Calling UpdateAccount will force a folder sync.
		client->UpdateAccount(payload, true);
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return msg->replySuccess();
}

MojErr PopBusDispatcher::AccountDeleted(MojServiceMessage* msg, MojObject& payload)
{
	// cacel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);
	MojLogInfo(s_log, "AccountDeleted's payload: %s", AsJsonString(payload).c_str());

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId, false);
		client->DeleteAccount(msg, payload);
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;
}

void PopBusDispatcher::SetupCleanupCallback()
{
	g_idle_add(&PopBusDispatcher::CleanupCallback, this);
}

gboolean PopBusDispatcher::CleanupCallback(void* data)
{
	if (data == NULL) {
		MojLogError(s_log, "PopBusDispatcher::CleanupCallback did not receive instance of PopBusDispatcher");
		return false;
	}

	PopBusDispatcher* popBusDispatcher = static_cast<PopBusDispatcher*>(data);
	popBusDispatcher->Cleanup();

	return false;
}

void PopBusDispatcher::Cleanup()
{
	for (ClientMap::iterator it = m_clients.begin(); it != m_clients.end(); it++) {
		ClientPtr client = (*it).second;
		if (client->IsAccountDeleted()) {
			m_clients.erase(it);
			break;
		}
	}
}

MojErr PopBusDispatcher::SyncFolder(MojServiceMessage* msg, MojObject& payload)
{
	// cacel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);
	MojLogInfo(s_log, "SyncFolder's payload: %s", AsJsonString(payload).c_str());

	try {
		// TODO: need to figure out how to get/create activity
		ActivityPtr activity; // = GetActivity(payload);

		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId, false);
		client->SyncFolder(payload);

		err = msg->replySuccess();
		ErrorToException(err);

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}
	return MojErrNone;
}

MojErr PopBusDispatcher::SyncAccount(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	try
	{
		MojErr err;
		MojObject accountId;
		err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId, false);

		client->SyncAccount(payload);

		err = msg->replySuccess();
		ErrorToException(err);

	}catch (const std::exception& e){
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}
	return MojErrNone;
}

MojErr PopBusDispatcher::MoveToSentFolder(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);
	MojLogInfo(s_log, "MoveToSentFolder's payload: %s", AsJsonString(payload).c_str());

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId, false);
		client->MoveOutboxFolderEmailsToSentFolder(msg, payload);
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;

}

MojErr PopBusDispatcher::MoveEmail(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);
	MojLogInfo(s_log, "MoveEmail's payload: %s", AsJsonString(payload).c_str());

	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId, false);
		client->MoveEmail(msg, payload);
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;
}

MojErr PopBusDispatcher::DownloadMessage(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);

	MojLogInfo(s_log, "Downloading message from client");
	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId, false);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		if(folderId.undefined() || folderId.null())
			MojErrThrowMsg(MojErrInternal, "Invalid folder ID");

		MojObject emailId;
		err = payload.getRequired("emailId", emailId);
		ErrorToException(err);

		MojObject partId(MojObject::TypeUndefined);

		boost::shared_ptr<DownloadListener> listener(new DownloadListener(msg, emailId, partId));
		client->DownloadPart(folderId, emailId, partId, listener);

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
	return MojErrNone;
}

MojErr PopBusDispatcher::DownloadMessageSubscribers()
{
	return MojErrNone;
}

MojErr PopBusDispatcher::DownloadAttachment(MojServiceMessage* msg, MojObject& payload)
{
	// cancel shut down if it is in shut down state
	CancelShutdown();

	MojLogTrace(s_log);

	MojLogInfo(s_log, "Downloading parts from client");
	try {
		MojObject accountId;
		MojErr err = payload.getRequired("accountId", accountId);
		ErrorToException(err);

		ClientPtr client = GetOrCreateClient(accountId, false);

		MojObject folderId;
		err = payload.getRequired("folderId", folderId);
		ErrorToException(err);

		if(folderId.undefined() || folderId.null())
			MojErrThrowMsg(MojErrInternal, "Invalid folder ID");

		MojObject emailId;
		err = payload.getRequired("emailId", emailId);
		ErrorToException(err);

		MojObject partId;
		err = payload.getRequired("partId", partId);
		ErrorToException(err);

		boost::shared_ptr<DownloadListener> listener(new DownloadListener(msg, emailId, partId));
		client->DownloadPart(folderId, emailId, partId, listener);

		return MojErrNone;
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "exception: %s", e.what());
	}
	return MojErrNone;
}

MojErr PopBusDispatcher::DownloadAttachmentSubscribers()
{
	return MojErrNone;
}

void PopBusDispatcher::GetAccountFromPayload(boost::shared_ptr<PopAccount>& account, MojObject& payload)
{
	MojString username;
	MojErr err = payload.getRequired("username", username);
	ErrorToException(err);
	account->SetUsername(username.data());

	MojString password;
	err = payload.getRequired("password", password);
	ErrorToException(err);
	account->SetPassword(password.data());

	// Protocol settings
	MojObject config;
	err = payload.getRequired("config", config);
	ErrorToException(err);

	// Optional username; overrides account username if available
	bool hasServerUsername = false;
	MojString serverUsername;
	err = config.get("username", serverUsername, hasServerUsername);
	ErrorToException(err);
	if (hasServerUsername) {
		account->SetUsername(serverUsername.data());
	}

	MojString server;
	err = config.getRequired("server", server);
	ErrorToException(err);
	account->SetHostName(server.data());

	int port;
	err = config.getRequired("port", port);
	ErrorToException(err);
	account->SetPort(port);

	MojString encryption;
	err = config.getRequired("encryption", encryption);
	ErrorToException(err);
	account->SetEncryption(encryption.data());
}

void PopBusDispatcher::GetAccountFromTransportObject(boost::shared_ptr<PopAccount>& account, MojObject& payload, MojObject& transportObj)
{
	MojString username;
	MojErr err = transportObj.getRequired("username", username);
	ErrorToException(err);
	account->SetUsername(username.data());

	MojString password;
	err = payload.getRequired("password", password);
	ErrorToException(err);
	account->SetPassword(password.data());

	MojString server;
	err = transportObj.getRequired("server", server);
	ErrorToException(err);
	account->SetHostName(server.data());

	int port;
	err = transportObj.getRequired("port", port);
	ErrorToException(err);
	account->SetPort(port);

	MojString encryption;
	err = transportObj.getRequired("encryption", encryption);
	ErrorToException(err);
	account->SetEncryption(encryption.data());
}

void PopBusDispatcher::PrepareShutdown()
{
	MojLogTrace(s_log);
	MojLogInfo(s_log, "Checking POP clients to see if POP service can be shut down");

	if (CanShutdown()) {
		MojLogInfo(s_log, "Setting up POP service shutdown sequence");
		SetupShutdownCallback();
	}
}

/**
 * Checks to see if POP transport can be shut down or not.
 */
bool PopBusDispatcher::CanShutdown()
{
	bool shutdown = true;
	ClientMap::iterator it;
	for (it = m_clients.begin(); it != m_clients.end(); ++it) {
		ClientPtr client = (*it).second;
		if (client->IsActive()) {
			shutdown = false;
			break;
		}
	}

	return shutdown;
}

void PopBusDispatcher::SetupShutdownCallback()
{
	// Wait 30 seconds to make sure that bush dispatcher can really be shut down.
	// If there is any request to the bus dispatcher within 30 seconds, the
	// shut down will be canclled.
	m_shutdownId = g_timeout_add_seconds(30, &PopBusDispatcher::ShutdownCallback, this);
}

void PopBusDispatcher::CancelShutdown()
{
	if (m_shutdownId > 0) {
		g_source_remove(m_shutdownId);
		MojLogInfo(s_log, "POP service shutdown attempt %d is cancelled", m_shutdownId);
		m_shutdownId = 0;
	}
}

gboolean PopBusDispatcher::ShutdownCallback(void* data)
{
	if (data == NULL) {
		MojLogError(s_log, "PopBusDispatcher::ShutdownCallback did not receive instance of PopBusDispatcher");
		return false;
	}

	try {
		PopBusDispatcher* popBusDispatcher = static_cast<PopBusDispatcher*> (data);

		if (!popBusDispatcher->CanShutdown()) {
			MojLogInfo(s_log, "There is outstanding work to do, so cancel POP transport shutdown");
			popBusDispatcher->m_shutdownId = 0;
		} else {
			MojLogInfo(s_log, "POP service is shutting down");
			popBusDispatcher->CompleteShutdown();
		}
	} catch (const std::exception& ex) {
		MojLogError(s_log, "Failed to shut down POP service: %s", ex.what());
	} catch (...) {
		MojLogError(s_log, "Unknown exception in shutting down POP service");
	}

	return false;
}

void PopBusDispatcher::CompleteShutdown()
{
	m_app.shutdown();
	MojLogInfo(s_log, "POP service shutdown sequence completes");
}

PopBusDispatcher::ClientPtr PopBusDispatcher::GetOrCreateClient(MojObject& accountId, bool sync)
{
	MojLogTrace(s_log);

	ClientPtr client;
	ClientMap::iterator it;

	it = m_clients.find(accountId);
	if (it == m_clients.end()) {
		MojLogInfo(s_log, "Creating PopClient");
		shared_ptr<DatabaseInterface> databaseInterface(new MojoDatabase(m_dbClient));
		client.reset(new PopClient(databaseInterface, this, accountId, &m_service));
		m_clients[accountId] = client;
		client->CreateSession();
		MojLogInfo(s_log, "Setting pop client with account id %s", AsJsonString(accountId).c_str());

	} else {
		client = it->second;
	}

	assert(client.get());
	MojObject payload;
	if (sync) {
		client->SyncAccount(payload);
	}

	return client;
}

MojErr PopBusDispatcher::Status(MojServiceMessage* msg, MojObject& payload)
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

		msg->replySuccess(status);

	} catch(const std::exception& e) {
		msg->replyError(MojErrInternal, e.what());
	} catch(...) {
		msg->replyError(MojErrInternal);
	}

	return MojErrNone;
}

MojErr PopBusDispatcher::Ping(MojServiceMessage* msg, MojObject& payload)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("ping request received."));

	bool subscribed = false;
	if (payload.get(_T("subscribe"), subscribed) && subscribed) {
		MojLogDebug(s_log, _T("ping subscription added."));
		MojRefCountedPtr<Subscription> sub(new Subscription(*this, msg));
		MojAllocCheck(sub.get());
		m_subscribers.push_back(sub.get());
		MojErr err = SetupTimer();
		ErrorToException(err);
	}
	return ReplyToPing(msg);
}

MojErr PopBusDispatcher::RemoveSubscription(Subscription* sub)
{
	MojLogTrace(s_log);
	MojLogDebug(s_log, _T("ping subscription canceled."));

	for (SubscriptionVec::iterator it = m_subscribers.begin();
		it != m_subscribers.end(); ++it) {
		if (it->get() == sub) {
			m_subscribers.erase(it);
			break;
		}
	}
	return MojErrNone;
}

MojErr PopBusDispatcher::ReplyToPing(MojServiceMessage* msg)
{
	MojLogTrace(s_log);

	MojObject reply;
	MojErr err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
	ErrorToException(err);
	err = reply.putString(_T("response"), _T("pong"));
	ErrorToException(err);
	err = msg->reply(reply);
	ErrorToException(err);
	return MojErrNone;
}

MojErr PopBusDispatcher::PingSubscribers()
{
	MojLogTrace(s_log);

	for (SubscriptionVec::const_iterator it = m_subscribers.begin();
		 it != m_subscribers.end(); it++) {
		MojErr err = ReplyToPing((*it)->msg().get());
		ErrorToException(err);
	}
	return MojErrNone;
}

MojErr PopBusDispatcher::SetupTimer()
{
	MojLogTrace(s_log);

	// only setup the timer if it's the first subscriber
	if (m_subscribers.size() == 1) {
		// Use the GLib main event loop to call us back every second
		g_timeout_add_seconds(1, &TimerCallback, this);
		return MojErrNone;
	}
	return MojErrNone;
}

gboolean PopBusDispatcher::TimerCallback(void* data)
{
	MojLogTrace(s_log);

	PopBusDispatcher* self = (PopBusDispatcher*) data;
	if (self->m_subscribers.empty()) {
		// if we don't have any subscribers, stop the timer
		return false;
	} else {
		self->PingSubscribers();
		return true;
	}
}

PopBusDispatcher::AccountCreator::AccountCreator(PopBusDispatcher& dispatcher,
												 MojServiceMessage* msg,
												 MojObject& payload)
: m_log("com.palm.pop"),
  m_dispatcher(dispatcher),
  m_msg(msg),
  m_payload(payload),
  m_createSlot(this, &PopBusDispatcher::AccountCreator::CreatePopAccountResponse),
  m_smtpAccountCreatedSlot(this, &PopBusDispatcher::AccountCreator::SmtpAccountCreatedResponse)
{

}

PopBusDispatcher::AccountCreator::~AccountCreator()
{

}

void PopBusDispatcher::AccountCreator::CreatePopAccount()
{
	MojLogTrace(m_log);
	MojLogInfo(m_log, "AccountCreator::CreatePopAccount");

	MojObject popAccount;
	PopAccountAdapter::GetPopAccountFromPayload(m_log, m_payload, popAccount);

	MojErr err = m_dispatcher.m_dbClient.put(m_createSlot, popAccount);

	MojLogInfo(m_log, "Created account in DB");

	ErrorToException(err);
}

MojErr PopBusDispatcher::AccountCreator::CreatePopAccountResponse(MojObject& response, MojErr err)
{
	MojLogInfo(m_log, "AccountCreator::CreatePopAccountResponse");

	MojLogTrace(m_log);
	if (err)
		return m_msg->replyError(err);

	MojString json;
	err = response.toJson(json);
	MojLogInfo(m_log, "%s", json.data());

	// put the id we have just gotten into the payload we send to smtp account
	MojObject resultArray;
	err = response.getRequired("results", resultArray);
	ErrorToException(err);
	MojObject result;
	resultArray.at(0, result);
	MojObject mailAccountId;
	err = result.getRequired("id", mailAccountId);
	ErrorToException(err);
	err = m_payload.put("mailAccountId", mailAccountId);
	ErrorToException(err);

	MojObject accountId;
	err = m_payload.getRequired("accountId", accountId);
	ErrorToException(err);

	ClientPtr client = m_dispatcher.GetOrCreateClient(accountId);
	client->SendRequest(m_smtpAccountCreatedSlot, "com.palm.smtp", "accountCreated", m_payload);

	return MojErrNone;
}

MojErr PopBusDispatcher::AccountCreator::SmtpAccountCreatedResponse(MojObject& response, MojErr err)
{
	if (err)
		return m_msg->replyError(err);

	return m_msg->replySuccess();
}

PopBusDispatcher::FirstUseValidator::FirstUseValidator(PopBusDispatcher& dispatcher,
													   MojoDatabase& dbInterface,
													   MojObject accountId,
													   MojServiceMessage* msg,
													   MojObject& payload)
: m_log("com.palm.pop"),
  m_dispatcher(dispatcher),
  m_dbInterface(dbInterface),
  m_accountId(accountId),
  m_msg(msg),
  m_payload(payload),
  m_findMailAccountSlot(this, &PopBusDispatcher::FirstUseValidator::FindMailAccountResponse)
{

}

void PopBusDispatcher::FirstUseValidator::Validate()
{
	m_dbInterface.GetAccount(m_findMailAccountSlot, m_accountId);
}

MojErr PopBusDispatcher::FirstUseValidator::FindMailAccountResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojObject accountObj;
		DatabaseAdapter::GetOneResult(response, accountObj);

		boost::shared_ptr<PopAccount> account(new PopAccount());
		m_dispatcher.GetAccountFromTransportObject(account, m_payload, accountObj);
		m_dispatcher.ValidatePopAccount(account, m_msg, m_payload);
	} catch (const std::exception& e) {
		MojErrThrowMsg(MojErrInternal, "%s", e.what());
	}

	return MojErrNone;
}

PopBusDispatcher::Subscription::Subscription(PopBusDispatcher& cat, MojServiceMessage* msg)
: m_cat(cat),
  m_msg(msg),
  m_cancelSlot(this, &Subscription::HandleCancel)
{
	MojLogTrace(s_log);

	msg->notifyCancel(m_cancelSlot);
}

PopBusDispatcher::Subscription::~Subscription()
{
	MojLogTrace(s_log);
}

MojErr PopBusDispatcher::Subscription::HandleCancel(MojServiceMessage* msg)
{
	MojLogTrace(s_log);

	return m_cat.RemoveSubscription(this);
}
