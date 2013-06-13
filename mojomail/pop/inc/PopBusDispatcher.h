// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#ifndef POPSERVICEHANDLER_H_
#define POPSERVICEHANDLER_H_

#include "activity/Activity.h"
#include "core/MojService.h"
#include "core/MojServiceMessage.h"
#include "db/MojDbServiceClient.h"
#include "luna/MojLunaService.h"
#include "network/SocketConnection.h"
#include "stream/LineReader.h"
#include "client/PopSession.h"
#include "glib.h"
#include "data/MojoDatabase.h"
#include "data/PopFolder.h"
#include "data/PopFolderAdapter.h"
#include "PopClient.h"
#include <map>
#include <string>
#include <vector>

class PopMain;

class PopBusDispatcher : public MojService::CategoryHandler
{
public:
	PopBusDispatcher(PopMain& app, MojDbServiceClient& dbClient,
					 MojLunaService& service);
	virtual ~PopBusDispatcher();
	MojErr Init();

	/**
	 * When a POP account has been completed deleted, POP client will use this
	 * function to let bus dispatcher to clean up the resources for this account.
	 */
	void SetupCleanupCallback();

	/**
	 * POP service can be shut down if all the POP clients have been idle for
	 * 30 seconds.
	 */
	void PrepareShutdown();

private:
	PopMain&									m_app;

	MojRefCountedPtr<CommandManager>			m_tasks;
	typedef MojRefCountedPtr<PopClient>			ClientPtr;
	typedef std::map<MojObject, ClientPtr>		ClientMap;

	class AccountCreator : public MojSignalHandler
	{
	public:
		AccountCreator(PopBusDispatcher& dispatcher,
					   MojServiceMessage* message,
					   MojObject& payload);
		~AccountCreator();

		void CreatePopAccount();
		MojLogger		m_log;

	private:
		MojErr CreatePopAccountResponse(MojObject& response, MojErr err);
		MojErr SmtpAccountCreatedResponse(MojObject& response, MojErr err);

		PopBusDispatcher& 							m_dispatcher;
		MojRefCountedPtr<MojServiceMessage>			m_msg;
		MojObject								 	m_payload;
		MojDbClient::Signal::Slot<AccountCreator> 	m_createSlot;
		MojDbClient::Signal::Slot<AccountCreator>	m_smtpAccountCreatedSlot;
	};

	class FirstUseValidator : public MojSignalHandler
	{
	public:
		FirstUseValidator(PopBusDispatcher& dispatcher,
						  MojoDatabase& dbInterface,
						  MojObject accountId,
						  MojServiceMessage* message,
						  MojObject& payload);

		void Validate();
		MojLogger m_log;

	private:
		MojErr FindMailAccountResponse(MojObject& response, MojErr err);

		PopBusDispatcher&								m_dispatcher;
		MojoDatabase&									m_dbInterface;
		MojObject										m_accountId;
		MojRefCountedPtr<MojServiceMessage>				m_msg;
		MojObject										m_payload;
		MojDbClient::Signal::Slot<FirstUseValidator> 	m_findMailAccountSlot;
	};

	class Subscription : public MojSignalHandler
	{
	public:
		Subscription(PopBusDispatcher& cat, MojServiceMessage* msg);
		~Subscription();

		MojRefCountedPtr<MojServiceMessage> msg() { return m_msg; }

	private:
		MojErr HandleCancel(MojServiceMessage* msg);

		PopBusDispatcher& m_cat;
		MojRefCountedPtr<MojServiceMessage> m_msg;
		MojServiceMessage::CancelSignal::Slot<Subscription> m_cancelSlot;
	};

	/**
	 * Database result of m_dbClientputKind from Init()
	 */
	MojErr PutKindResult(MojObject& result, MojErr err);

	/**
	 * palm://com.palm.pop/validateAccount
	 *
	 * Using the protocol settings object, attempts to validate the .
	 *
	 * @param ProtocolSettings JSON object
	 * @return if account can be validated, "returnValue": true and credentials
	 *         if account cannot be validated, "returnValue": false and error/error string set
	 * @see https://wiki.palm.com/display/Nova/Validation+API
	 */
	MojErr ValidateAccount(MojServiceMessage* msg, MojObject& payload);
	void   ValidatePopAccount(boost::shared_ptr<PopAccount> account, MojRefCountedPtr<MojServiceMessage> msg, MojObject& payload);

	/**
	 * palm://com.palm.pop/viewEmlMessage
	 */
	MojErr ParseEmlMessage(MojServiceMessage* msg, MojObject& payload);

	/**
	 * palm://com.palm.pop/accountCreated
	 *
	 * Stores config object from callee if necessary; responds to notification that account with
	 * account ID provided has been created.
	 *
	 * @param {"accountID":<Mojo DB ID of account>, "config":<config object>}
	 * @return if account can be validated, "returnValue": true and credentials
	 *         if account cannot be validated, "returnValue": false and error/error string set
	 * @see https://wiki.palm.com/display/Bedlam/Accounts+Framework+Interface#AccountsFrameworkInterface-{{onCreate}}
	 */
	MojErr AccountCreated(MojServiceMessage* msg, MojObject& payload);

	/**
	 * palm://com.palm.pop/accountEnabled
	 *
	 * If enabled is set to true, starts off an initial sync, schedule future syncs. If enabled
	 * is set to false, then disable future syncs and delete PIM data associated to account.
	 *
	 * @param {"enabled":true|false, "accountID":<Mojo DB ID of account>}
	 * @return {"returnValue":true|false}
	 * @see https://wiki.palm.com/display/Bedlam/Accounts+Framework+Interface#AccountsFrameworkInterface-{{onEnabled}}
	 */
	MojErr AccountEnabled(MojServiceMessage* msg, MojObject& payload);

	MojErr AccountUpdated(MojServiceMessage* msg, MojObject& payload);

	/**
	 * palm://com.palm.pop/accountDeleted
	 *
	 * Delete all account-wide config data.
	 *
	 * @param {"accountID":<Mojo DB ID of account>}
	 * @return {"returnValue":true|false}
	 * @see https://wiki.palm.com/display/Bedlam/Accounts+Framework+Interface#AccountsFrameworkInterface-{{onDelete}}
	 */
	MojErr AccountDeleted(MojServiceMessage* msg, MojObject& payload);
	MojErr CredentialsChanged(MojServiceMessage* msg, MojObject& payload);

	static gboolean CleanupCallback(void* data);
	void Cleanup();

	MojErr SyncFolder(MojServiceMessage* msg, MojObject& payload);
	MojErr SyncAccount(MojServiceMessage* msg, MojObject& payload);

	MojErr MoveToSentFolder(MojServiceMessage* msg, MojObject& payload);
	MojErr MoveEmail(MojServiceMessage* msg, MojObject& payload);

	MojErr DownloadMessage(MojServiceMessage* msg, MojObject& payload);
	MojErr DownloadMessageSubscribers();

	MojErr DownloadAttachment(MojServiceMessage* msg, MojObject& payload);
	MojErr DownloadAttachmentSubscribers();

	void GetAccountFromPayload(boost::shared_ptr<PopAccount>& account, MojObject& payload);
	void GetAccountFromTransportObject(boost::shared_ptr<PopAccount>& account, MojObject& payload, MojObject& transportObj);

	/**
	 * Checks to see if POP transport can be shut down or not.
	 */
	bool CanShutdown();
	
	/**
	 * Sets up a 30 seconds wait period to make sure that there are no new request
	 * that should be handled by POP service.  If there are new requests within
	 * this period, the shutdown process will be cancelled.
	 */
	void   SetupShutdownCallback();
	/**
	 * Cancels shut down process since there is new request.
	 */
	void   CancelShutdown();
	/**
	 * A callback function for g_timeout_add to invoke when the maximum idle time
	 * has been reached.
	 */
	static gboolean ShutdownCallback(void* data);
	/**
	 * Completes the shutdown of POP service.
	 */
	void   CompleteShutdown();

	ClientPtr GetOrCreateClient(MojObject& accountId, bool sync = false);

	/**
	 * palm://com.palm.pop/ping
	 *
	 * Method simply returns a "pong" response.
	 *
	 * @param {}
	 * @return {"returnValue":true, "response":"pong}
	 */
	MojErr Ping(MojServiceMessage* msg, MojObject& payload);
	MojErr RemoveSubscription(Subscription* sub);
	MojErr ReplyToPing(MojServiceMessage* msg);
	MojErr PingSubscribers();
	MojErr SetupTimer();

	ActivityPtr GetActivity(MojObject& payload);

	/**
	 * palm://com.palm.pop/status
	 *
	 * Return status of POP service, as noted in @return section.
	 *
	 * @param {}
	 * @return @see https://wiki.palm.com/display/Nova/Mail+Transport+Interface
	 */
	MojErr Status(MojServiceMessage* msg, MojObject& payload);
	MojErr ReplyToStatus(MojServiceMessage* msg);

	/**
	 * GLib callback for the Ping timer.
	 * This callback is invoked every second when we have subscribers to the Ping method.
	 *
	 * @see http://library.gnome.org/devel/glib/unstable/glib-The-Main-Event-Loop.html#g-timeout-add-seconds
	 */
	static gboolean TimerCallback(void* data);

	ClientMap						m_clients;
	/**
	 * List of methods available on the bus.
	 */
	static const Method 			s_methods[];
	static const char* const 		POP_KIND;
	/**
	 * Sys logger.
	 */
	static MojLogger 				s_log;

	/**
	 * Database client used to make any requests (put, watch, find, etc.) to Mojo DB
	 */
	MojDbServiceClient&		 			m_dbClient;

	MojoDatabase						m_dbInterface;

	/**
	 * Pointer to the service used for creating/sending requests.
	 */
	MojLunaService&					m_service;

	/**
	 * Pointer to a BusClient
	 */

	boost::shared_ptr<BusClient>						m_bClient;

	/**
	 * An ID that is returned from g_timeout_add function.  A non-zero value indicates
	 * that a shutdown is in progress.
	 */
	guint							m_shutdownId;

	typedef MojRefCountedPtr<Subscription>	SubscriptionPtr;
	typedef std::vector<SubscriptionPtr>	SubscriptionVec;
	SubscriptionVec m_subscribers;

	/**
	 * Database slot used for the response of the putKind operation called from Init()
	 */
	MojDbClient::Signal::Slot<PopBusDispatcher> m_putKindSlot;
};

#endif /* POPSERVICEHANDLER_H_ */
