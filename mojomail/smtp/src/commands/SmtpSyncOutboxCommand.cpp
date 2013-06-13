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

#include "commands/SmtpSyncOutboxCommand.h"
#include "SmtpClient.h"
#include "data/EmailSchema.h"
#include <boost/regex.hpp>
#include <boost/foreach.hpp>
#include "data/SmtpAccountAdapter.h"
#include "activity/ActivityBuilder.h"
#include "activity/SmtpActivityFactory.h"
#include "data/SyncStateAdapter.h"
#include "data/EmailAccountAdapter.h"
#include "data/DatabaseAdapter.h"

// TODO: Need to use ActivitySet for activity hanlding
SmtpSyncOutboxCommand::SmtpSyncOutboxCommand(SmtpClient& client, const MojObject& accountId, const MojObject& folderId, bool force, bool clear)
: SmtpCommand(client),
  m_client(client),
  m_accountId(accountId),
  m_folderId(folderId),
  m_highestRev(0),
  m_canAdopt(true),
  m_setSyncStatusSlot(this, &SmtpSyncOutboxCommand::SetSyncStatusResponse),
  m_clearSyncStatusSlot(this, &SmtpSyncOutboxCommand::ClearSyncStatusResponse),
  m_setSyncStatusSlot2(this, &SmtpSyncOutboxCommand::SetSyncStatusResponse2),
  m_clearSyncStatusSlot2(this, &SmtpSyncOutboxCommand::ClearSyncStatusResponse2),
  m_getOutboxEmailsSlot(this, &SmtpSyncOutboxCommand::GetOutboxEmailsResponse),
  m_sendDoneSlot(this, &SmtpSyncOutboxCommand::SendDone),
  m_activityUpdatedSlot(this, &SmtpSyncOutboxCommand::ActivityUpdated),
  m_activityErrorSlot(this, &SmtpSyncOutboxCommand::ActivityError),
  m_accountActivityUpdatedSlot(this, &SmtpSyncOutboxCommand::AccountActivityUpdated),
  m_accountActivityErrorSlot(this, &SmtpSyncOutboxCommand::AccountActivityError),
  m_getAccountSlot(this, &SmtpSyncOutboxCommand::GotAccount),
  m_temporaryError(false),
  m_createWatchActivitySlot(this, &SmtpSyncOutboxCommand::CreateWatchActivityResponse),
  m_createAccountWatchActivityResponseSlot(this, &SmtpSyncOutboxCommand::CreateAccountWatchResponse),
  m_findFolderSlot(this, &SmtpSyncOutboxCommand::FindOutgoingFolderResponse),
  m_updateRetrySlot(this, &SmtpSyncOutboxCommand::UpdateRetryResponse),
  m_updateErrorSlot(this, &SmtpSyncOutboxCommand::UpdateErrorResponse),
  m_resetErrorSlot(this, &SmtpSyncOutboxCommand::ResetErrorResponse),
  m_cancelled(false),
  m_force(force),
  m_clear(clear),
  m_networkStatus(new NetworkStatus()),
  m_networkActivityUpdatedSlot(this, &SmtpSyncOutboxCommand::NetworkActivityUpdated),
  m_networkActivityErrorSlot(this, &SmtpSyncOutboxCommand::NetworkActivityError)
{
	MojString d;
	m_accountId.stringValue(d);
	m_outboxWatchActivityName.format(SmtpActivityFactory::OUTBOX_WATCH_ACTIVITY_FMT, d.data());
	m_accountWatchActivityName.format(SmtpActivityFactory::ACCOUNT_WATCH_ACTIVITY_FMT, d.data());

#ifdef WEBOS_TARGET_MACHINE_STANDALONE
	MojObject cmStatus;
	MojErr err;
	err = cmStatus.put("isInternetConnectionAvailable", true);
    ErrorToException(err);
    m_networkStatus->ParseStatus(cmStatus);
#endif

}

SmtpSyncOutboxCommand::~SmtpSyncOutboxCommand()
{
}

void SmtpSyncOutboxCommand::AttachAdoptedActivity(MojRefCountedPtr<Activity> activity, const MojString& activityId, const MojString& activityName)
{
	MojLogInfo(m_log, "Attaching adopted activity, incoming activityId=%s, name=%s. WatchActivityName=%s"
		, activityId.data(), activityName.data(), m_outboxWatchActivityName.data());

	if (activityName == m_outboxWatchActivityName) {
		if(m_outboxWatchActivity.get() && activity.get()) {
			MojLogWarning(m_log, "%s: outbox activity already attached; replacing activity %s with %s", __PRETTY_FUNCTION__, AsJsonString(m_outboxWatchActivity->GetActivityId()).c_str(), AsJsonString(activity->GetActivityId()).c_str());
		}

		m_outboxWatchActivity = activity;
	} else if (activityName == m_accountWatchActivityName) {
		if(m_accountWatchActivity.get() && activity.get()) {
			MojLogWarning(m_log, "%s: account watch activity already attached; replacing activity %s with %s", __PRETTY_FUNCTION__, AsJsonString(m_accountWatchActivity->GetActivityId()).c_str(), AsJsonString(activity->GetActivityId()).c_str());
		}

		m_accountWatchActivity = activity;
	} else {
		m_manualActivities.push_back( activity );
	}
	
	m_networkStatus->ParseActivity(activity);
}

MojObject SmtpSyncOutboxCommand::GetAccountId()
{
	return m_accountId;
}

bool SmtpSyncOutboxCommand::CanAdopt()
{
	return m_canAdopt && !m_cancelled;
}

void SmtpSyncOutboxCommand::Cancel()
{
	m_cancelled = true;
}

/**
 * Send flow
 *  - Get folder ID from account, if not provided
 *  - request list of mails that might need to be sent
 *  - for any mail that needs to be sent, enqueue a send mail command
 *  - if a send mail command has an error, mark account as needing a retry after a timeout, and stop mail sync.
 *  - loop on list of mails. At end of list, refetch list if we made any changes.
 *
 *
 *  - Error processing: mark account as needing a try after a timeoutAdopt activity from trigger OR create activity (manual sync)
 *  - Send emails
 *  - Create new watch activity
 *  - End old activity
 */
void SmtpSyncOutboxCommand::RunImpl()
{
	try {
		m_client.GetTempDatabaseInterface().ClearSyncStatus(m_clearSyncStatusSlot, m_accountId, m_folderId);
	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
}

MojErr SmtpSyncOutboxCommand::ClearSyncStatusResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		m_client.GetTempDatabaseInterface().CreateSyncStatus(m_setSyncStatusSlot, m_accountId, m_folderId, SyncStateAdapter::STATE_INCREMENTAL_SYNC);

		MojLogInfo(m_log, "SmtpSyncOutboxCommand::ClearSyncStatusResponse old SMTP sync status cleared, creating new sync status");
	} catch (std::exception& e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}

	return MojErrNone;
}

MojErr SmtpSyncOutboxCommand::SetSyncStatusResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);
		
		MojLogInfo(m_log, "SmtpSyncOutboxCommand::SetSyncStatusResponse sync status created");

		StartSync();
	} catch (std::exception& e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}

	return MojErrNone;
}

void SmtpSyncOutboxCommand::StartSync()
{
	if (m_clear) {
		MojLogInfo(m_log, "Clearing account before running sync");
		m_client.GetSession()->ClearAccount();
	}

	MojString folderIdJson;
	m_folderId.toJson(folderIdJson);
	MojLogInfo(m_log, "syncing outbox %s", folderIdJson.data());

	m_error.clear();

	// Set up error code that will be logged on account if we fall over without managing to elsewise
	// report an error code -- just in case.
	m_error.errorCode = MailError::INTERNAL_ERROR;
	m_error.errorOnAccount = true;
	m_error.errorOnEmail = false;
	m_error.internalError = "Unknown error in smtp outbox processing";
	m_error.errorText = "";

	// FIXME: Shouldn't this be done before we reply to the sync
	// message, or adopt the activity, or something? There's a
	// lot of hysteresis in sleep, so this is unlikely to be
	// critical, it just feels wrong to do this this late,
	// without overlapping it over anything else.
	m_client.GetPowerManager().StayAwake(true, "smtp outbox sync");

	CheckNetworkConnectivity();
}

void SmtpSyncOutboxCommand::CheckNetworkConnectivity()
{
	try {

		if (m_networkStatus->IsKnown()) {
			if (m_networkStatus->IsConnected()) {
				MojLogInfo(m_log, "CheckNetworkActivity: is connected, moving on");
				GetAccount();
				return;
			} else {
				MojLogInfo(m_log, "CheckNetworkActivity: is not connected, erroring out");
				// Note that this account error explicitly doesn't delay the retry, on the assumption
				// that the next activity will be blocked until the network is available.
				m_error.errorCode = MailError::NO_NETWORK;
				m_error.errorOnAccount = true;
				m_error.errorOnEmail = false;
				m_error.internalError = "No network available for outbox sync";
				m_error.errorText = "";

				CompleteAndUpdateActivities();
				return;
			}
		} else {
			MojLogInfo(m_log, "CheckNetworkActivity: state unknown, starting new activity");
			MojLogInfo(m_log, "OutboxSyncer creating new network activity");
			ActivityBuilder ab;
			MojString name;
			MojErr err = name.format("SMTP Internal Outbox Sync Network Activity for account %s", AsJsonString(m_accountId).c_str());
			ErrorToException(err);
			ab.SetName(name);
			ab.SetDescription("Activity representing SMTP Outbox Sync Network Monitor");
			ab.SetForeground(true);
			ab.SetRequiresInternet(false);
			ab.SetImmediate(true, ActivityBuilder::PRIORITY_LOW);
			m_networkActivity = Activity::PrepareNewActivity(ab);
			m_networkActivity->SetSlots(m_networkActivityUpdatedSlot, m_networkActivityErrorSlot);
			m_networkActivity->Create(m_client);
		}

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
}

MojErr SmtpSyncOutboxCommand::NetworkActivityUpdated(Activity * activity, Activity::EventType)
{
	try {
		MojLogInfo(m_log, "SyncOutboxcommand has updated network activity");
		
		
		MojRefCountedPtr<Activity> actPtr = activity;

		MojLogInfo(m_log, "Activity->info is %s", AsJsonString(actPtr->GetInfo()).c_str());

		bool p = m_networkStatus->ParseActivity(actPtr);
		
		MojLogInfo(m_log, "p=%d, known=%d, connected=%d",
			p, m_networkStatus->IsKnown(), m_networkStatus->IsConnected());

		if (m_networkStatus->IsKnown()) {
			m_networkActivityUpdatedSlot.cancel();
			m_networkActivityErrorSlot.cancel();
        
			// Go on
			CheckNetworkConnectivity();
		}

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
        
	return MojErrNone;
}

MojErr SmtpSyncOutboxCommand::NetworkActivityError(Activity * activity, Activity::ErrorType, const std::exception& exc)
{
	try {
		MojLogInfo(m_log, "SyncOutboxCommand has network activity error");
		
		m_networkActivityUpdatedSlot.cancel();
		m_networkActivityErrorSlot.cancel();

		m_error.errorCode = MailError::NO_NETWORK;
		m_error.errorOnAccount = true;
		m_error.errorOnEmail = false;
		m_error.internalError = "No network available for outbox sync";
		m_error.errorText = "";

		CompleteAndUpdateActivities();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
        
	return MojErrNone;
}

void SmtpSyncOutboxCommand::GetAccount()
{
	try {

		m_client.GetDatabaseInterface().GetAccount(m_getAccountSlot, m_accountId);

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
}

MojErr SmtpSyncOutboxCommand::GotAccount(MojObject &response, MojErr err)
{
	try {
		ResponseToException(response, err);
		MojLogInfo(m_log, "GotAccount, response elided");
	
		try {
			MojObject accountObj;
			DatabaseAdapter::GetOneResult(response, accountObj);
                                                                                                                                        
			err = accountObj.getRequired(EmailAccountAdapter::OUTBOX_FOLDERID, m_folderId);
			ErrorToException(err);

			// FIXME _ prefix is reserved for MojoDB internal properties
			err = accountObj.getRequired("_revSmtp", m_accountRev);
			ErrorToException(err);
			
			MojLogInfo(m_log, "Sync GotAccount, _revSmtp=%lld", m_accountRev);
			
			boost::shared_ptr<SmtpAccount> smtpAccountObj(new SmtpAccount());
			
			MojObject nothing;
			
			SmtpAccountAdapter::GetSmtpAccount(nothing, accountObj, smtpAccountObj);
			
			m_error = SmtpSession::SmtpError(smtpAccountObj->GetSmtpError());
			
			FindOutgoingFolder();

		} catch (const std::exception& e) {
	
			MojLogInfo(m_log, "No account data, erroring out: %s", e.what());
			
			// We must log an error if we cannot find the
			// appropriate outbox folder id, so that in the case
			// of a missing folder, we'll disconnect any
			// potentially left-over watches, and not come back
			// up until the account watches trigger us to update
			// and try again.
			
			m_error.errorCode = MailError::SMTP_CONFIG_UNAVAILABLE;
			m_error.errorOnAccount = true;
			m_error.errorOnEmail = false;
			m_error.internalError = "Unable to retrieve account data for SMTP account";
			m_error.errorText = "";
			
			SmtpSyncOutboxCommand::CompleteAndUpdateActivities();
			
			return MojErrNone;
		}

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
	
	return MojErrNone;
}

void SmtpSyncOutboxCommand::FindOutgoingFolder()
{
	try {
		m_client.GetDatabaseInterface().GetFolder(m_findFolderSlot, m_accountId, m_folderId);

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
}

MojErr SmtpSyncOutboxCommand::FindOutgoingFolderResponse(MojObject &response, MojErr err)
{
	try {
		ResponseToException(response, err);
		MojLogInfo(m_log, "FindOutgoingFolderResponse, response elided");
	
		try {
			MojObject folder;
			DatabaseAdapter::GetOneResult(response, folder);

			m_retryDelay.clear();
			folder.get("smtpRetryDelay", m_retryDelay);
			ErrorToException(err);
                
			m_retryDelayNew = m_retryDelay;

			MojString json;
			m_retryDelay.toJson(json);
			MojLogInfo(m_log, "got retry delay %s", json.data());
			
			UpdateAccountWatchActivity();
		
		} catch(...) {
			MojLogInfo(m_log, "No outgoing folder, erroring out");
		
			// We must log an error if we cannot find the
			// appropriate outbox folder id, so that in the case
			// of a missing folder, we'll disconnect any
			// potentially left-over watches, and not come back
			// up until the account watches trigger us to update
			// and try again.
			
			m_error.errorCode = MailError::SMTP_OUTBOX_UNAVAILABLE;
			m_error.errorOnAccount = true;
			m_error.errorOnEmail = false;
			m_error.internalError = "Unable to establish outbox folder associated with SMTP account";
			m_error.errorText = "";
			SmtpSyncOutboxCommand::CompleteAndUpdateActivities();
			
			return MojErrNone;
		}

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
        
	return MojErrNone;
}

void SmtpSyncOutboxCommand::UpdateAccountWatchActivity()
{
	MojLogInfo(m_log, "UpdatingAccountWatchActivity");
	
	try {
		// accoundId json object
		MojString accountIdJson;
		MojErr err = m_accountId.toJson(accountIdJson);
		ErrorToException(err);

		SmtpActivityFactory factory;
		ActivityBuilder ab;

		factory.BuildSmtpConfigWatch(ab, m_accountId, m_accountRev);

		bool updating = m_accountWatchActivity.get();

		// and either update and complete the updated activity if we had adopted it, or re-create it.
		
		if ( updating ) {
			
			MojLogInfo(m_log, "updating account watch activity");
	
			m_accountWatchActivity->SetSlots(m_accountActivityUpdatedSlot, m_accountActivityErrorSlot);
	
			m_accountWatchActivity->UpdateAndComplete(m_client, ab.GetActivityObject());
			
		} else {
			// Create payload
			MojObject payload;
			err = payload.put("activity", ab.GetActivityObject());
			ErrorToException(err);
			err = payload.put("start", true);
			ErrorToException(err);
			err = payload.put("replace", true);
			ErrorToException(err);
				
			MojLogInfo(m_log, "creating account watch activity");
							
			m_client.SendRequest(m_createAccountWatchActivityResponseSlot, "com.palm.activitymanager", "create", payload);
		}

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
}

MojErr SmtpSyncOutboxCommand::AccountActivityUpdated(Activity * activity, Activity::EventType)
{
	try {
		MojLogInfo(m_log, "SyncOutboxcommand has updated account activity");

		// FIXME: should drop activity from vector

		m_accountActivityUpdatedSlot.cancel();
		m_accountActivityErrorSlot.cancel();
        
		// Go on
		CheckErrorCodes();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
        
	return MojErrNone;
}

MojErr SmtpSyncOutboxCommand::AccountActivityError(Activity * activity, Activity::ErrorType, const std::exception& exc)
{
	try {
		MojLogInfo(m_log, "SyncOutboxCommand has account activity error");

		// FIXME: should drop activity from vector

		m_accountActivityUpdatedSlot.cancel();
		m_accountActivityErrorSlot.cancel();

		// go on
		CheckErrorCodes();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
        
	return MojErrNone;
}

MojErr SmtpSyncOutboxCommand::CreateAccountWatchResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojString json;
		err = response.toJson(json);
		MojErrCheck(err);
		MojLogInfo(m_log, "response from activitymanager on updating watch activity: %s", json.data());

		CheckErrorCodes();

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}

	return MojErrNone;
}

void SmtpSyncOutboxCommand::CheckErrorCodes()
{
	CommandTraceFunction();
	try {
		
		if (m_error.IsLockoutError()) {
			if (m_force) {
				MojLogInfo(m_log, "Account locked out due to error (code=%d,internal=%s), but continuing due to force.",
					m_error.errorCode,
					m_error.internalError.c_str());
			} else {
				MojLogInfo(m_log, "Account locked out due to error (code=%d,internal=%s), completing sync early.",
					m_error.errorCode,
					m_error.internalError.c_str());
				SmtpSyncOutboxCommand::CompleteAndUpdateActivities();
				return;
			}
		}
		
		if (m_error.errorCode) {
			// Reset error code before we continue -- if the
			// account still has trouble, we'll put the error
			// back before we're done.
			
			// clear our state
			m_error.clear();
			
			MojString text;
			text.assign("");
			// clear the db state
			m_client.GetDatabaseInterface().UpdateAccountErrorStatus(m_resetErrorSlot, m_accountId, 0, text);
			return;
		} else {
			// clear entire error state
			m_error.clear();
		}
		
		GetOutboxEmails();
	
	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
}

MojErr SmtpSyncOutboxCommand::ResetErrorResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojString json;
		response.toJson(json);		
		MojLogInfo(m_log, "SmtpSyncOutboxCommand::ResetErrorResponse got response to updating error status, payload=%s", json.data());
	
		GetOutboxEmails();

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}

	return MojErrNone;
}


void SmtpSyncOutboxCommand::GetOutboxEmails()
{
	CommandTraceFunction();
	try {

		m_outboxPage.clear();
		m_emailsToSend.clear();

		GetSomeOutboxEmails();
	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
}

void SmtpSyncOutboxCommand::GetSomeOutboxEmails()
{
	// Fetch emails from the outbox
	m_client.GetDatabaseInterface().GetOutboxEmails(m_getOutboxEmailsSlot, m_folderId, m_outboxPage);
}

MojErr SmtpSyncOutboxCommand::GetOutboxEmailsResponse(MojObject &response, MojErr err)
{
	try {
		ResponseToException(response, err);
	
		try {
			ErrorToException(err);

			BOOST_FOREACH(const MojObject& email, DatabaseAdapter::GetResultsIterators(response)) {
				m_emailsToSend.push_back(email);
			}

			if(DatabaseAdapter::GetNextPage(response, m_outboxPage)) {
				// Get more emails
				GetSomeOutboxEmails();
			} else {
				MojLogInfo(m_log, "Found %d emails in outbox", m_emailsToSend.size());

				m_emailIt = m_emailsToSend.begin();

				m_didSomething = false;

				SendNextEmail();
			}
		} catch(std::exception& e) {
			HandleException(e, __func__);
		} catch(...) {
			HandleException(__func__);
		}

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
	
	return MojErrNone;
}

void SmtpSyncOutboxCommand::SendNextEmail()
{
	CommandTraceFunction();
	try {
	
		bool sendEmailPending = false;
	
		// Fire off a command for the *first* email eligible for sending, in _rev order.
		// We don't skip individual mails if they recently failed -- we skip the entire
		// account in that case, until it's due for a retry.
		
		while (m_emailIt != m_emailsToSend.end() && !m_cancelled) {
			MojObject &email = *m_emailIt;

			MojErr err;
			MojObject id;
			err = email.getRequired("_id", id);
			ErrorToException(err);

			// Update highest rev
			MojInt64 rev;
			err = email.getRequired("_rev", rev);
			ErrorToException(err);

			if(rev > m_highestRev)
				m_highestRev = rev;
		
			bool fatalError = false;
			bool sent = false;
		
			// make sure mail is 'unsent'
			MojObject sendStatus;
			if(email.get(EmailSchema::SEND_STATUS, sendStatus) && !sendStatus.null()) {
				// Check if the email has already been sent
				sendStatus.get(EmailSchema::SendStatus::SENT, sent);
				
				// Check if the email can't be sent
				sendStatus.get(EmailSchema::SendStatus::FATAL_ERROR, fatalError);
			}

			// Advance iterator to next email
			++m_emailIt;
		
			MojLogInfo(m_log, "Inspecting email %s rev=%lld, sent=%d, fatalError=%d", AsJsonString(id).c_str(), rev, sent ? 1 : 0, fatalError ? 1 : 0);
			
			if(!fatalError && !sent) {
				m_client.GetSession()->SendMail(id, m_sendDoneSlot);
				sendEmailPending = true;
				m_didSomething = true;
				break;
			}
		}
		
		if(!sendEmailPending) {
			// If there's no more emails pending, set up our watch, and don't allow
			// any more activities to be adopted by us -- they'll need to start up a new
			// session.
		
			if (m_didSomething) {
				MojLogInfo(m_log, "Looping, checking for more mail");
				
				// Did we do anything? If so, start over from the beginning, looping
				// until we _don't_ have anything to do.
				
				GetOutboxEmails();
			} else {
				MojLogInfo(m_log, "No more mail, shutting down connection, updating activities");
				
				m_canAdopt = false;
				
				CompleteAndUpdateActivities();
			}
		} else {
			MojLogInfo(m_log, "Waiting for mail send to complete");
		}

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
}


MojErr SmtpSyncOutboxCommand::SendDone(SmtpSession::SmtpError error)
{
	CommandTraceFunction();

	try {

		if (error.errorCode || error.errorOnAccount || error.errorOnEmail) {
			MojLogError(m_log, "SmtpSyncOutboxCommand:SendDone with error! (errorCode=%d errorText=%s internalError=%s errorOnAccount=%d errorOnEmail=%d)",
					error.errorCode,
					error.errorText.c_str(),
					error.internalError.c_str(),
					error.errorOnAccount ? 1 : 0,
					error.errorOnEmail ? 1 : 0);
		} else {
			MojLogInfo(m_log, "SmtpSyncOutboxCommand:SendDone success!");
		}
	
		if (error.errorOnAccount) {
			MojLogInfo(m_log, "Due to account error, completing early");
			m_error = error;
			CompleteAndUpdateActivities();
			return MojErrNone;
		}
		
		SendNextEmail();

	} catch (std::exception & e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}
	
	return MojErrNone;
}

// Create or update the DB watch activity
void SmtpSyncOutboxCommand::CompleteAndUpdateActivities()
{
	CommandTraceFunction();

	try {

		MojLogInfo(m_log, "SmtpSyncOutboxCommand::CompleteAndUpdateActivities");
	
		MojLogInfo(m_log, " manual activities size=%d", m_manualActivities.size());
	
		// Clear any network activity
		if (m_networkActivity.get()) {
			m_networkActivity->End(m_client);
			m_networkActivity.reset();
		}
	
		int j = 0;
		for (std::vector<MojRefCountedPtr<Activity> >::iterator iter=m_manualActivities.begin();
			iter != m_manualActivities.end(); 
			iter++, j++) {
			MojLogInfo(m_log, " manual activity %d active=%d", j, (*iter)->IsActive());
			if ((*iter)->IsActive()) {
				MojLogInfo(m_log, " completing manual activity %d", j);
				(*iter)->SetSlots(m_activityUpdatedSlot, m_activityErrorSlot);
				(*iter)->Complete(m_client);
				return;
			}
		}
	
		MojLogInfo(m_log, " outboxWatchActivity=%p, isActive=%d", m_outboxWatchActivity.get(), 
			m_outboxWatchActivity.get() ? m_outboxWatchActivity->IsActive() : -1);
		
		

		if (!m_folderId.undefined() && !m_folderId.null() && (!m_outboxWatchActivity.get() || m_outboxWatchActivity->IsActive())) {
			MojLogInfo(m_log, " completing watch activity");
		
		
			bool updating = m_outboxWatchActivity.get();
		
			MojLogInfo(m_log, " updating = %d", updating ? 1 : 0);

			// Next recompute what our watch needs to look like

			if (m_error.errorOnAccount && m_error.errorCode != MailError::NO_NETWORK && m_error.IsLockoutError()) {
		
				// We don't want to retry on an account credentials error, so shut down our watches.
				// We'll expected either a forced manual sync to force us to sync again, or an
				// update on the account/credentials, which also triggers a forced sync, which will
				// then reset the error state.
				
				// If the network wasn't available, then don't treat it as an error at all -- we'll block
				// on the normal activity until the network is actually available.
				
				if ( updating ) {

					MojLogInfo(m_log, "completing outbox watch activity by id, removing it due to account error");
	
					m_outboxWatchActivity->SetSlots(m_activityUpdatedSlot, m_activityErrorSlot);
	
					m_outboxWatchActivity->Complete(m_client);
		
				} else {
					// Create payload
					MojObject payload;
					MojErr err = payload.put("activityName", m_outboxWatchActivityName);
					ErrorToException(err);
				
					MojLogInfo(m_log, "completing outbox watch activity by name, removing it due to account error");
				
					m_client.SendRequest(m_createWatchActivitySlot, "com.palm.activitymanager", "cancel", payload);
				}

			} else {
	
				// Normal case, no error
	
				// Activity
				SmtpActivityFactory factory;
				ActivityBuilder actBuilder;

				factory.BuildOutboxWatch(actBuilder, m_accountId, m_folderId);

				if (m_error.errorOnAccount) {
					// Account had error, not individual email: schedule outbox sync based on retry delay
			
					int r = m_retryDelayNew.intValue();
			
					// FIXME: set up retry delays going from 30 seconds doubling to 5 minutes
					// Needs to be double-checked against Java, and put in a central place.
				
					if (r < 30) {
						r = 30;
					} else {
						r = r * 2;
						if (r > 300)
							r = 300;
					}
				
					m_retryDelayNew = MojObject(r);
				
					MojLogNotice(m_log, "rescheduling outbox watch for %d seconds in the future", (int)m_retryDelayNew.intValue());
					actBuilder.SetSyncInterval(m_retryDelayNew.intValue(), 0);
					actBuilder.SetRequiresInternetConfidence("fair");
					if (updating)
						actBuilder.DisableDatabaseWatchTrigger();
			
				} else {
					// no error, use db watch to trigger outbox sync
				
					// Query
					MojDbQuery query;
					query.from(EmailSchema::Kind::EMAIL);
					query.where(EmailSchema::FOLDER_ID, MojDbQuery::OpEq, m_folderId);
					if(m_highestRev > 0)
						query.where("_rev", MojDbQuery::OpGreaterThan, m_highestRev);
					
					MojLogInfo(m_log, "Setting outbox watch rev to %lld", m_highestRev);
		
					// Trigger
					actBuilder.SetDatabaseWatchTrigger(query);
					if (updating)
						actBuilder.DisableSyncInterval();
				
					m_retryDelayNew.clear();
				}

				// and either update and complete the updated activity if we had adopted it, or re-create it.
				if ( updating ) {
			
					MojLogInfo(m_log, "updating outbox watch activity");
		
					m_outboxWatchActivity->SetSlots(m_activityUpdatedSlot, m_activityErrorSlot);
		
					m_outboxWatchActivity->UpdateAndComplete(m_client, actBuilder.GetActivityObject());
			
				} else {
					// Create payload
					MojObject payload;
					MojErr err;

					err = payload.put("activity", actBuilder.GetActivityObject());
					ErrorToException(err);
					err = payload.put("start", true);
					ErrorToException(err);
					err = payload.put("replace", true);
					ErrorToException(err);
				
					MojLogInfo(m_log, "creating outbox watch activity");
				
					m_client.SendRequest(m_createWatchActivitySlot, "com.palm.activitymanager", "create", payload);
				}
			
			}
			
		} else {
				MojLogInfo(m_log, " activity completion done, moving on");
			
		        ActivityCompleted();
		}

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
}

MojErr SmtpSyncOutboxCommand::ActivityUpdated(Activity * activity, Activity::EventType)
{
	try {
		MojLogInfo(m_log, "SyncOutboxcommand has updated activity");

		// FIXME: should drop activity from vector

		m_activityUpdatedSlot.cancel();
		m_activityErrorSlot.cancel();
        
		// Go back, see if there are more activities to update or complete
		CompleteAndUpdateActivities();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
        
	return MojErrNone;
}

MojErr SmtpSyncOutboxCommand::ActivityError(Activity * activity, Activity::ErrorType, const std::exception& exc)
{
	try {
		MojLogInfo(m_log, "SyncOutboxCommand has activity error");

		// FIXME: should drop activity from vector

		m_activityUpdatedSlot.cancel();
		m_activityErrorSlot.cancel();

		// Go back, see if there are more activities to update or complete
		CompleteAndUpdateActivities();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
        
	return MojErrNone;
}

MojErr SmtpSyncOutboxCommand::CreateWatchActivityResponse(MojObject& response, MojErr err)
{
	try {
		if(err != MojErrNotFound) {
			ResponseToException(response, err);
		}

		MojString json;
		response.toJson(json);		
		MojLogInfo(m_log, "SyncOutboxcommand got response to activity creation, payload=%s", json.data());

		//we're done
		ActivityCompleted();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}

	return MojErrNone;
}

void SmtpSyncOutboxCommand::ActivityCompleted()
{
	try {
		if (m_retryDelayNew != m_retryDelay) {
			MojString json;
			m_retryDelayNew.toJson(json);
			MojLogInfo(m_log, "SmtpSyncOutboxCommand updating retry to %s", json.data());
			m_client.GetDatabaseInterface().UpdateFolderRetry(m_updateRetrySlot, m_folderId, m_retryDelayNew);
			m_retryDelay = m_retryDelayNew;
			return;
		}
		
		ActivityCompleted2();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
}


MojErr SmtpSyncOutboxCommand::UpdateRetryResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojString json;
		response.toJson(json);		
		MojLogInfo(m_log, "SyncOutboxcommand got response to updating retry delay, payload=%s", json.data());
	
		ActivityCompleted2();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}

	return MojErrNone;
}

void SmtpSyncOutboxCommand::ActivityCompleted2()
{
	try {
		MojLogInfo(m_log, "SyncOutboxcommand ActivityCompleted2");
	
		if (m_error.errorCode != 0 && !m_error.IsTrivialError()) {
			MojLogInfo(m_log, "SmtpSyncOutboxCommand updating account error status to code=%d/text=%s/internalError=%s", m_error.errorCode, m_error.errorText.c_str(), m_error.internalError.c_str());
			MojObject code(m_error.errorCode);
			MojString text;
			text.assign(m_error.errorText.c_str());
			m_client.GetDatabaseInterface().UpdateAccountErrorStatus(m_updateErrorSlot, m_accountId, code, text);
			return;
		}
	
		Done();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}
}

MojErr SmtpSyncOutboxCommand::UpdateErrorResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojString json;
		response.toJson(json);		
		MojLogInfo(m_log, "SyncOutboxcommand got response to updating error status, payload=%s", json.data());
	
		Done();

	} catch (std::exception & e) {
		HandleException(e, __func__, true);
	} catch (...) {
		HandleException(__func__, true);
	}

	return MojErrNone;
}

void SmtpSyncOutboxCommand::HandleException(std::exception & e, const char * func, bool late)
{
	std::string m = "Internal exception at ";
	m += func;
	m += ": ";
	m += e.what();
	MojLogInfo(m_log, "%s", m.c_str());
	if (!late) {
		MojLogInfo(m_log, "Trying to complete, store error state and update activities");
		CompleteAndUpdateActivities();
	} else {
		MojLogInfo(m_log, "Completing command early, too late to be graceful");
		Done();
	}
}

void SmtpSyncOutboxCommand::HandleException(const char * func, bool late)
{
	std::string m = "Internal exception at ";
	m += func;
	MojLogInfo(m_log, "%s", m.c_str());
	if (!late) {
		MojLogInfo(m_log, "Trying to complete, store error state and update activities");
		CompleteAndUpdateActivities();
	} else {
		MojLogInfo(m_log, "Completing command early, too late to be graceful");
		Done();
	}
}

void SmtpSyncOutboxCommand::Done()
{
	MojLogInfo(m_log, "SyncOutboxcommand Done");
	// remove syncStatus from tempdb

	m_client.GetTempDatabaseInterface().ClearSyncStatus(m_clearSyncStatusSlot2, m_accountId, m_folderId);
}

MojErr SmtpSyncOutboxCommand::ClearSyncStatusResponse2(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		m_client.GetTempDatabaseInterface().CreateSyncStatus(m_setSyncStatusSlot2, m_accountId, m_folderId, SyncStateAdapter::STATE_IDLE);
	} catch (std::exception& e) {
		HandleException(e, __func__);
	} catch (...) {
		HandleException(__func__);
	}

	return MojErrNone;
}

MojErr SmtpSyncOutboxCommand::SetSyncStatusResponse2(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojLogInfo(m_log, "SmtpSyncOutboxCommand::SetSyncStatusResponse2 SMTP sync status updated");
	} catch (...) {
		MojLogInfo(m_log, "SmtpSyncOutboxCommand::SetSyncStatusResponse2 exception trying to clear sync status");
	}

	//we're done
	try {
		m_client.CommandComplete(this);
	} catch (...) {
		MojLogInfo(m_log, "SmtpSyncOutboxCommand::SetSyncStatusResponse2 exception trying to get sync status & completing command");
	}

	try {
		m_client.GetPowerManager().StayAwake(false, "smtp outbox sync");
	} catch  (...) {
		MojLogInfo(m_log, "SmtpSyncOutboxCommand::SetSyncStatusResponse2 exception trying to release power");
	}

	return MojErrNone;
}

void SmtpSyncOutboxCommand::Status(MojObject& status) const
{
	MojErr err;
	SmtpCommand::Status(status);

	if(m_error.errorCode != MailError::NONE) {
		MojObject errorStatus;
		m_error.Status(errorStatus);

		err = status.put("m_error", errorStatus);
		ErrorToException(err);
	}
}
