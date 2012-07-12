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

#include "commands/IdleYahooCommand.h"
#include "ImapPrivate.h"
#include "client/ImapSession.h"
#include "data/ImapAccount.h"
#include "client/FolderSession.h"
#include "commands/SyncEmailsCommand.h"

// How long to wait for the Yahoo service to acknowledge our subscribe request
const int IdleYahooCommand::SERVICE_RESPONSE_TIMEOUT = 60; // one minute

IdleYahooCommand::IdleYahooCommand(ImapSession& session, const MojObject& folderId)
: BaseIdleCommand(session),
  m_subscribedYahoo(false),
  m_syncInProgress(false),
  m_folderId(folderId),
  m_syncSlot(this, &IdleYahooCommand::SyncDone),
  m_subscriptionResponseSlot(this, &IdleYahooCommand::SubscriptionResponse)
{
}

IdleYahooCommand::~IdleYahooCommand()
{
}

void IdleYahooCommand::RunImpl()
{
	CommandTraceFunction();

	boost::shared_ptr<FolderSession> folderSession = m_session.GetFolderSession();
	if(folderSession.get() && folderSession->HasUIDMap()) {
		// If we have a UID map, we probably sync'd recently
		SubscribeWakeup();
	} else {
		InitialSync();
	}
}

void IdleYahooCommand::InitialSync()
{
	// If not, kick off a sync
	m_syncCommand.reset(new SyncEmailsCommand(m_session, m_folderId));
	m_syncCommand->Run(m_syncSlot);
}

MojErr IdleYahooCommand::SyncDone()
{
	CommandTraceFunction();

	try {
		m_syncCommand.reset();

		// FIXME: don't go into idle if there's queued commands after the sync
		SubscribeWakeup();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void IdleYahooCommand::SubscribeWakeup()
{
	CommandTraceFunction();

	try {
		MojErr err;
		MojObject params;

		MojObject accountId = m_session.GetAccount()->GetId();
		err = params.put(ImapAccountAdapter::ACCOUNT_ID, accountId);
		ErrorToException(err);
		err = params.putBool("subscribe", true);
		ErrorToException(err);
		MojLogInfo(m_log, "Subscribing Yahoo Service");

		m_subscribeTimer.SetTimeout(SERVICE_RESPONSE_TIMEOUT, this, &IdleYahooCommand::SubscribeTimeout);

		m_session.GetClient()->SendRequest(m_subscriptionResponseSlot, "com.palm.yahoo", "monitorchanges", params, MojServiceRequest::Unlimited);
	} CATCH_AS_FAILURE
}

void IdleYahooCommand::SubscribeTimeout()
{
	CommandTraceFunction();

	MojLogCritical(m_log, "Yahoo service didn't respond to subscribe request within %d seconds; aborting", SERVICE_RESPONSE_TIMEOUT);

	MailException exc("timed out waiting for Yahoo service to respond", __FILE__, __LINE__);

	Failure(exc);
}

MojErr IdleYahooCommand::SubscriptionResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	MojLogInfo(m_log, "subscription response");

	m_subscribeTimer.Cancel();

	try {
		ResponseToException(response, err);
		// Notification received, trigger a sync.

		bool retValue;
		err = response.getRequired("returnValue", retValue);
		ErrorToException(err);

		bool newMailReceived = false;
		bool bGotNewMail = response.get("newMail", newMailReceived);

		if(retValue && !bGotNewMail && !m_subscribedYahoo) {
			MojLogInfo(m_log, "start idling");
			m_subscribedYahoo = true;
			m_session.IdleStarted(true);
		} else if(retValue && (bGotNewMail && newMailReceived) && !m_syncInProgress) {
			m_syncInProgress = true;
			MojLogInfo(m_log, "received notification from Yahoo service; starting sync");
			SyncParams syncParams;
			m_session.SyncFolder(m_folderId, syncParams);
		} else if(!retValue) {
			// FIXME retry subscription.
		}

	} CATCH_AS_FAILURE
	return MojErrNone;
}


void IdleYahooCommand::EndIdle()
{
	if(!m_endingIdle) {
		CommandTraceFunction();

		m_endingIdle = true;

		try {
			MojLogInfo(m_log, "EndIdle called, unsubscribing Yahoo service.");
			m_subscriptionResponseSlot.cancel();
			m_subscribedYahoo = false;

			m_session.IdleDone();
			Complete();
		} CATCH_AS_FAILURE
	} else {
		MojLogInfo(m_log, "already ending idle");
	}
}

void IdleYahooCommand::Failure(const exception& e)
{
	m_session.IdleError(e, false); // FIXME figure what which errors are fatal
	m_session.IdleDone();

	ImapSessionCommand::Failure(e);
}

void IdleYahooCommand::Cleanup()
{
	m_subscriptionResponseSlot.cancel();
}

void IdleYahooCommand::Status(MojObject& status) const
{
	MojErr err;

	ImapSessionCommand::Status(status);

	err = status.put("subscribed", m_subscribedYahoo);
	ErrorToException(err);

	if(m_syncCommand.get()) {
		MojObject commandStatus;
		m_syncCommand->Status(commandStatus);

		err = status.put("syncCommand", commandStatus);
		ErrorToException(err);
	}
}

std::string IdleYahooCommand::Describe() const
{
	return ImapSessionCommand::Describe(m_folderId);
}
