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

#ifndef SMTPBUSDISPATCHER_H_
#define SMTPBUSDISPATCHER_H_

#include "SmtpCommon.h"
#include "SmtpClient.h"
#include "SmtpValidator.h"
#include "SmtpSimpleSender.h"
#include "core/MojService.h"
#include "luna/MojLunaService.h"
#include <map>
#include "data/MojoDatabase.h"
#include "db/MojDbClient.h"
#include "db/MojDbServiceClient.h"

class SmtpServiceApp;

class SmtpBusDispatcher : public MojService::CategoryHandler
{
public:
	static MojLogger s_log;

	SmtpBusDispatcher(SmtpServiceApp& app);
	virtual ~SmtpBusDispatcher();
	
	MojErr InitHandler();
	
	MojErr Ping(MojServiceMessage* msg, MojObject& payload);
	MojErr Status(MojServiceMessage* msg, MojObject& payload);
	MojErr ValidateAccount(MojServiceMessage* msg, MojObject& payload);
	MojErr AccountCreated(MojServiceMessage* msg, MojObject& payload);
	MojErr AccountEnabled(MojServiceMessage* msg, MojObject& payload);
	MojErr AccountUpdated(MojServiceMessage* msg, MojObject& payload);
	MojErr AccountDeleted(MojServiceMessage* msg, MojObject& payload);
	MojErr SimpleSendEmail(MojServiceMessage* msg, MojObject& payload);
	MojErr SendMail(MojServiceMessage* msg, MojObject& payload);
	MojErr SaveDraft(MojServiceMessage* msg, MojObject& payload);
	MojErr SyncOutbox(MojServiceMessage* msg, MojObject& payload);

	void PrepareShutdown();

private:

	typedef	MojRefCountedPtr<SmtpValidator> 		ValidatorPtr;
	typedef MojRefCountedPtr<SmtpClient>			ClientPtr;
	typedef std::map<MojObject, ClientPtr>		 	ClientMap;

	class Validator : public MojSignalHandler
	{
	public:
		Validator(SmtpBusDispatcher& dispatcher,
				  MojoDatabase& dbInterface,
				  MojServiceMessage* message,
				  MojObject& payload);

		void Start();
		MojLogger m_log;

	private:
		MojErr FindMailAccountResponse(MojObject& response, MojErr err);
		void Validate(MojObject& accountObj);
		SmtpBusDispatcher&								m_dispatcher;
		MojoDatabase&									m_dbInterface;
		MojRefCountedPtr<MojServiceMessage>				m_msg;
		MojObject										m_payload;
		MojDbClient::Signal::Slot<Validator> 			m_findMailAccountSlot;
	};

	class AccountCreator : public MojSignalHandler
	{
	public:
		AccountCreator(SmtpBusDispatcher& dispatcher,
					   MojServiceMessage* msg,
					   MojObject& payload);
		~AccountCreator();

		void CreateSmtpAccount();

	private:
		MojErr MergeSmtpConfigResponse(MojObject& response, MojErr err);

		MojLogger& 									m_log;
		SmtpBusDispatcher& 							m_dispatcher;
		MojRefCountedPtr<MojServiceMessage> 		m_msg;
		MojObject 									m_payload;
		MojDbClient::Signal::Slot<AccountCreator> 	m_mergeSlot;
	};

	class OutboxWatchCreator : public MojSignalHandler
	{
	public:
		OutboxWatchCreator(SmtpBusDispatcher& dispatcher,
						   MojServiceMessage* msg,
						   MojObject& payload);
		~OutboxWatchCreator();

		void CreateOutboxWatch();

	private:
		MojErr GetAccountResponse(MojObject& response, MojErr err);
		MojErr CreateOutboxWatchResponse(MojObject& response, MojErr err);

		void CreateWatchActivity(const MojObject& transportObj);

		std::auto_ptr<SmtpAccount>						m_account;
		MojLogger&										m_log;
		SmtpBusDispatcher&								m_dispatcher;
		MojRefCountedPtr<MojServiceMessage>				m_msg;
		MojObject										m_payload;
		MojDbClient::Signal::Slot<OutboxWatchCreator>	m_getAccountResponseSlot;
		MojDbClient::Signal::Slot<OutboxWatchCreator>	m_createOutboxWatchSlot;
	};

	class AccountUpdater : public MojSignalHandler
	{
	public:
		AccountUpdater(	SmtpBusDispatcher& dispatcher,
						ClientPtr smtpClient,
						MojServiceMessage* msg,
						MojObject& accountId,
						MojObject& payload);
		~AccountUpdater();

		void UpdateAccount();

	private:
		MojErr ActivityUpdated(Activity *, Activity::EventType);
		MojErr ActivityError(Activity *, Activity::ErrorType, const std::exception&);

		std::auto_ptr<SmtpAccount>						m_account;
		MojLogger&										m_log;
		SmtpBusDispatcher&								m_dispatcher;
		ClientPtr										m_accountClient;
		MojRefCountedPtr<MojServiceMessage>				m_msg;
		MojObject										m_accountId;
		MojObject										m_payload;
		MojString										m_activityId;
		MojString										m_activityName;
		MojRefCountedPtr<Activity>						m_activity;
		Activity::UpdateSignal::Slot<AccountUpdater>	m_activityUpdatedSlot;
		Activity::ErrorSignal::Slot<AccountUpdater>		m_activityErrorSlot;
	};

	class OutboxSyncer : public MojSignalHandler
	{
	public:
		OutboxSyncer(SmtpBusDispatcher& dispatcher,
						   ClientPtr smtpClient,
						   MojServiceMessage* msg,
						   MojObject& accountId,
						   MojObject& folderId,
						   bool force,
						   MojObject& payload);
		~OutboxSyncer();

		void SyncOutbox();
		
	private:
		MojErr ActivityUpdated(Activity *, Activity::EventType);
		MojErr ActivityError(Activity *, Activity::ErrorType, const std::exception&);

		std::auto_ptr<SmtpAccount>						m_account;
		MojLogger&										m_log;
		SmtpBusDispatcher&								m_dispatcher;
		ClientPtr										m_accountClient;
		MojRefCountedPtr<MojServiceMessage>				m_msg;
		MojObject										m_accountId;
		MojObject										m_folderId;
		bool											m_force;
		MojObject										m_payload;
		MojString										m_activityId;
		MojString										m_activityName;
		MojRefCountedPtr<Activity>						m_activity;
		Activity::UpdateSignal::Slot<OutboxSyncer>		m_activityUpdatedSlot;
		Activity::ErrorSignal::Slot<OutboxSyncer>		m_activityErrorSlot;
	};

	SmtpServiceApp &m_app;
	
	ClientPtr		GetOrCreateClient(MojObject& accountId);
	ClientPtr 		CreateClient();
	MojErr 			RegisterMethods();

	void			CancelShutdown();
	static gboolean ShutdownCallback(void* data);
	void 			Shutdown();

	void 			ValidateSmtpAccount(boost::shared_ptr<SmtpAccount> account, MojRefCountedPtr<MojServiceMessage> msg, MojObject& payload);
	void 			GetAccountFromTransportObject(MojObject& payload, MojObject& transportObj, boost::shared_ptr<SmtpAccount>& account);

	boost::shared_ptr<SmtpSession> 		m_session;
	ClientMap							m_clients;
	MojDbServiceClient&					m_dbClient;
	MojoDatabase						m_dbInterface;
	BusClient							m_client;
	boost::shared_ptr<CommandManager>	m_tasks;
	guint								m_shutdownId;
};

#endif /*SMTPBUSDISPATCHER_H_*/
