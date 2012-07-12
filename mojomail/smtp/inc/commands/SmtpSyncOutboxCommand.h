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

#ifndef SMTPSYNCOUTBOXCOMMAND_H_
#define SMTPSYNCOUTBOXCOMMAND_H_

#include "SmtpCommand.h"
#include "SmtpClient.h"
#include "db/MojDbClient.h"
#include "activity/Activity.h"
#include "activity/NetworkStatus.h"
#include <boost/regex.hpp>

class SmtpSyncOutboxCommand : public SmtpCommand
{
public:
	SmtpSyncOutboxCommand(SmtpClient& client, const MojObject& folderId, const MojObject& accountId, bool force, bool clear);
	virtual ~SmtpSyncOutboxCommand();
	
	void AttachAdoptedActivity(MojRefCountedPtr<Activity> activity, const MojString& activityId, const MojString& activityName);

	virtual void RunImpl();
	virtual void Cancel();
	
	MojObject GetAccountId();
	bool CanAdopt();
	
	void Status(MojObject& status) const;

protected:
	MojErr	ClearSyncStatusResponse(MojObject& response, MojErr err);
	MojErr	SetSyncStatusResponse(MojObject& response, MojErr err);
	void	StartSync();
	MojErr 	ClearSyncStatusResponse2(MojObject& response, MojErr err);
	MojErr	SetSyncStatusResponse2(MojObject& response, MojErr err);

	void	GetOutboxEmails();
	void	GetSomeOutboxEmails();
	MojErr	GetOutboxEmailsResponse(MojObject& response, MojErr err);
	void	SendNextEmail();
	MojErr	SendDone(SmtpSession::SmtpError);
	MojErr	SendTemporaryError();
	
	// These methods are used to set up a watch without syncing the outbox
	void	GetAccount();
	MojErr	GotAccount(MojObject& response, MojErr err);
	
	void CompleteAndUpdateActivities();
	
	void FindOutgoingFolder();
	MojErr FindOutgoingFolderResponse(MojObject &response, MojErr err);

	void UpdateAccountWatchActivity();
	MojErr	AccountActivityUpdated(Activity * activity, Activity::EventType);
	MojErr	AccountActivityError(Activity * activity, Activity::ErrorType, const std::exception& exc);
	MojErr CreateAccountWatchResponse(MojObject& response, MojErr err);
	
	MojErr	ActivityUpdated(Activity * activity, Activity::EventType);
	MojErr	ActivityError(Activity * activity, Activity::ErrorType, const std::exception& exc);
	
	void CheckNetworkConnectivity();
	
	MojErr	NetworkActivityUpdated(Activity * activity, Activity::EventType);
	MojErr	NetworkActivityError(Activity * activity, Activity::ErrorType, const std::exception& exc);
	
	MojErr	CreateWatchActivityResponse(MojObject& response, MojErr err);
	MojErr	UpdateRetryResponse(MojObject& response, MojErr err);

	void CheckErrorCodes();

	MojErr	UpdateErrorResponse(MojObject& response, MojErr err);
	MojErr	ResetErrorResponse(MojObject& response, MojErr err);
	
	void HandleException(const char * func, bool late=false);
	void HandleException(std::exception & e, const char * func, bool late=false);
	
	virtual void ActivityCompleted();
	virtual void ActivityCompleted2();
	virtual void Done();
	
	SmtpClient&	m_client;
	MojObject	m_accountId;
	MojObject	m_folderId;
	MojInt64	m_highestRev;
	bool		m_outboxNeedsWatch;

	std::vector<MojObject>	m_emailsToSend;
	std::vector<MojObject>::iterator	m_emailIt;

	bool		m_canAdopt;
	bool		m_didSomething;
	MojObject	m_retryDelay;
	MojObject	m_retryDelayNew;
	int		m_accountError;
	
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_setSyncStatusSlot;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_clearSyncStatusSlot;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_setSyncStatusSlot2;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_clearSyncStatusSlot2;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_getOutboxEmailsSlot;
	MojSignal<SmtpSession::SmtpError>::Slot<SmtpSyncOutboxCommand>	m_sendDoneSlot;
	Activity::UpdateSignal::Slot<SmtpSyncOutboxCommand>				m_activityUpdatedSlot;
	Activity::ErrorSignal::Slot<SmtpSyncOutboxCommand>				m_activityErrorSlot;
	Activity::UpdateSignal::Slot<SmtpSyncOutboxCommand>				m_accountActivityUpdatedSlot;
	Activity::ErrorSignal::Slot<SmtpSyncOutboxCommand>				m_accountActivityErrorSlot;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_getAccountSlot;

 	MojString	m_outboxWatchActivityName;
 	MojString	m_accountWatchActivityName;
 	
	MojRefCountedPtr<Activity> m_outboxWatchActivity;
	MojRefCountedPtr<Activity> m_accountWatchActivity;
	MojRefCountedPtr<Activity> m_networkActivity;
	std::vector<MojRefCountedPtr<Activity> > m_manualActivities;
	bool		m_temporaryError;
	MojServiceRequest::ReplySignal::Slot<SmtpSyncOutboxCommand>		m_createWatchActivitySlot;
	MojServiceRequest::ReplySignal::Slot<SmtpSyncOutboxCommand>		m_createAccountWatchActivityResponseSlot;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_findFolderSlot;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_updateRetrySlot;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_updateErrorSlot;
	MojDbClient::Signal::Slot<SmtpSyncOutboxCommand>				m_resetErrorSlot;
	SmtpSession::SmtpError m_error;
	bool		m_cancelled;
	bool		m_force;
	MojInt64	m_accountRev;
	bool		m_clear;

	MojDbQuery::Page	m_outboxPage;

	boost::shared_ptr<NetworkStatus>	m_networkStatus;
	Activity::UpdateSignal::Slot<SmtpSyncOutboxCommand>				m_networkActivityUpdatedSlot;
	Activity::ErrorSignal::Slot<SmtpSyncOutboxCommand>				m_networkActivityErrorSlot;
};

#endif /*SMTPSYNCOUTBOXCOMMAND_H_*/
