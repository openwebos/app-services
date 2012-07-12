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

#ifndef IMAPBUSDISPATCHER_H_
#define IMAPBUSDISPATCHER_H_

#include "core/MojObject.h"
#include "core/MojRefCount.h"
#include "core/MojService.h"
#include "db/MojDbClient.h"
#include "luna/MojLunaService.h"
#include "ImapValidator.h"
#include "ImapClient.h"
#include <map>
#include <vector>
#include <string>
#include "data/MojoDatabase.h"
#include "activity/Activity.h"
#include "client/BusClient.h"
#include "network/NetworkStatusMonitor.h"
#include "commands/ImapBusDispatcherCommand.h"

class ImapServiceApp;
class ImapSession;
class CommandManager;

class ImapBusDispatcher : public MojService::CategoryHandler, public ImapValidationListener, public BusClient, public Command::Listener
{
public:
	ImapBusDispatcher(ImapServiceApp& app);
	virtual ~ImapBusDispatcher();
	
	MojErr InitHandler();
	
	MojErr ping(MojServiceMessage* msg, MojObject& payload);
	
	// Called by ImapClient to indicate if the client is active/inactive
	void	UpdateClientActive(ImapClient* client, bool active);

	NetworkStatusMonitor& GetNetworkStatusMonitor() const { return *m_networkStatusMonitor; }

	void CommandComplete(Command* command);

private:

	typedef	MojRefCountedPtr<ImapValidator> 		ValidatorPtr;
	typedef MojRefCountedPtr<ImapClient>			ClientPtr;
	typedef std::map<MojObject, ClientPtr>		 	ClientMap;

	/**
	 * The AccountCreator is responsible for creating the com.palm.imap.account:1
	 * transport object when an account is created.
	 */
	class AccountCreator : public ImapBusDispatcherCommand
	{
	public:
		AccountCreator(ImapBusDispatcher& dispatcher, MojLogger& logger,
					   MojServiceMessage* message,
					   MojObject& payload);
		virtual ~AccountCreator();

		void RunImpl();
		void Status(MojObject& status) const;

	private:
		MojErr GetAccountResponse(MojObject& response, MojErr err);
		MojErr PersistAccountResponse(MojObject& response, MojErr err);
		MojErr SmtpAccountCreatedResponse(MojObject& response, MojErr err);

		MojRefCountedPtr<MojServiceMessage>			m_message;
		MojObject								 	m_payload;
		MojServiceRequest::ReplySignal::Slot<AccountCreator>	m_getAccountSlot;
		MojDbClient::Signal::Slot<AccountCreator> 	m_persistAccountSlot;

		MojServiceRequest::ReplySignal::Slot<AccountCreator>	m_smtpAccountCreatedSlot;
	};

	class StatusSubscription : public MojSignalHandler {
	public:
		StatusSubscription(ImapBusDispatcher& busDispatcher, MojServiceMessage* msg)
		: m_dispatcher(busDispatcher),
		  m_message(msg),
		  m_cancelSlot(this, &StatusSubscription::handleCancel)
		{
			msg->notifyCancel(m_cancelSlot);
		}
		virtual ~StatusSubscription() {}

		const MojRefCountedPtr<MojServiceMessage>& GetServiceMessage() const { return m_message; }

	protected:
		MojErr handleCancel(MojServiceMessage* msg) { m_dispatcher.StatusUnsubscribe(this); return MojErrNone; }

		ImapBusDispatcher& m_dispatcher;
		MojRefCountedPtr<MojServiceMessage> m_message;
		MojSignal<MojServiceMessage*>::Slot<StatusSubscription>	m_cancelSlot;
	};

	void Status(MojObject& status) const;
	void StatusUpdate();
	void StatusSubscribe(MojServiceMessage* msg);
	void StatusUnsubscribe(StatusSubscription* subscription);

	friend class StatusSubscription;

	MojErr ValidateAccount(MojServiceMessage*, MojObject&);
	MojErr AccountCreated(MojServiceMessage* msg, MojObject& payload);
	MojErr AccountEnabled(MojServiceMessage* msg, MojObject& payload);
	MojErr AccountUpdated(MojServiceMessage* msg, MojObject& payload);
	MojErr AccountDeleted(MojServiceMessage* msg, MojObject& payload);
	MojErr CredentialsChanged(MojServiceMessage* msg, MojObject& payload);
	MojErr RegisterMethods();

	MojErr StatusRequest(MojServiceMessage* msg, MojObject& payload);
	MojErr SyncAccount(MojServiceMessage* msg, MojObject& payload);
	MojErr SentEmails(MojServiceMessage* msg, MojObject& payload);
	MojErr DraftSaved(MojServiceMessage* msg, MojObject& payload);
	MojErr SyncFolder(MojServiceMessage* msg, MojObject& payload);
	MojErr SyncUpdate(MojServiceMessage* msg, MojObject& payload);
	MojErr DownloadMessage(MojServiceMessage* msg, MojObject& payload);
	MojErr DownloadAttachment(MojServiceMessage* msg, MojObject& payload);
	MojErr SearchFolder(MojServiceMessage* msg, MojObject& payload);
	MojErr WakeupIdle(MojServiceMessage* msg, MojObject& payload);
	MojErr StartIdle(MojServiceMessage* msg, MojObject& payload);

	// Debug method
	MojErr MagicWand(MojServiceMessage* msg, MojObject& payload);

	ClientPtr		GetOrCreateClient(MojObject& accountId);

	ActivityPtr 	GetActivity(MojObject& payload);

	// Are there any clients active
	bool	HasActiveClients();

	// Update inactivity shutdown timer as needed
	void	CheckActive();

	static gboolean InactivityCallback(gpointer data);

	// Called by ImapValidator
	void	ValidationDone(ImapValidator* validator);

	static gboolean	CleanupValidatorsCallback(gpointer data);

	ImapServiceApp &m_app;

	MojLogger		m_log;

	ClientMap							m_clients;
	MojoDatabase						m_dbInterface;

	std::vector<ValidatorPtr>			m_validators;
	std::vector<ValidatorPtr>			m_completedValidators;

	guint								m_cleanupValidatorsCallbackId;
	guint								m_inactivityCallbackId;

	MojRefCountedPtr<CommandManager>	m_commandManager;

	MojRefCountedPtr<NetworkStatusMonitor>					m_networkStatusMonitor;

	std::vector< MojRefCountedPtr<StatusSubscription> >		m_statusSubscriptions;
};

#endif /*IMAPBUSDISPATCHER_H_*/
