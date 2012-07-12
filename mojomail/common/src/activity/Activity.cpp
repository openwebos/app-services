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

#include "activity/Activity.h"
#include "activity/ActivityBuilder.h"
#include "client/BusClient.h"
#include "CommonPrivate.h"
#include <sstream>

MojLogger Activity::s_log("com.palm.mail.activity");

Activity::Activity()
: m_state(s_NeedsCreation),
  m_purpose(GenericActivity),
  m_endAction(EndAction_Unsubscribe),
  m_endOrder(EndOrder_Default),
  m_isParent(false),
  m_subscriptionSlot(this, &Activity::CreateOrAdoptResponse),
  m_completedSlot(this, &Activity::CompleteResponse),
  m_replaceSlot(this, &Activity::ReplaceResponse),
  m_cancelSlot(this, &Activity::CancelResponse),
  m_updateSignal(this),
  m_errorSignal(this)
{
}

Activity::~Activity()
{
}

void Activity::PutIdOrName(MojObject& payload)
{
	MojErr err;

	if(!m_activityId.undefined() && !m_activityId.null())
		err = payload.put("activityId", m_activityId);
	else if(!m_activityName.empty())
		err = payload.put("activityName", m_activityName);
	else
		throw MailException("need activityId or name", __FILE__, __LINE__);
}

MojRefCountedPtr<Activity> Activity::PrepareNewActivity(ActivityBuilder& builder, bool start, bool replace)
{
	ActivityPtr activity(new Activity());
	activity->m_state = s_NeedsCreation;

	MojObject obj;
	MojErr err = obj.put("activity", builder.GetActivityObject());
	ErrorToException(err);
	err = obj.putBool("start", start);
	ErrorToException(err);
	err = obj.putBool("replace", replace);
	ErrorToException(err);
	err = obj.putBool("detailedEvents", true);
	ErrorToException(err);
	activity->m_newActivity = obj;

	return activity;
}

MojRefCountedPtr<Activity> Activity::PrepareAdoptedActivity(const MojObject& activityId)
{
	ActivityPtr activity(new Activity());
	activity->m_state = s_NeedsAdoption;
	activity->m_activityId = activityId;
	return activity;
}

MojRefCountedPtr<Activity> Activity::PrepareAdoptedActivity(const MojString& name)
{
	ActivityPtr activity(new Activity());
	activity->m_state = s_NeedsAdoption;
	activity->m_activityName = name;
	return activity;
}

void Activity::SetSlots(UpdateSignal::SlotRef updateSlot, ErrorSignal::SlotRef errorSlot) {
	m_updateSignal.connect(updateSlot);
	m_errorSignal.connect(errorSlot);
}

const char* Activity::GetEventTypeName(EventType type)
{
	switch(type) {
	case StartEvent:
		return "StartEvent";
	case OrphanEvent:
		return "OrphanEvent";
	case UpdateEvent:
		return "UpdateEvent";
	case CompleteEvent:
		return "CompleteEvent";
	case StopEvent:
		return "StopEvent";
	case CancelEvent:
		return "CancelEvent";
	case YieldEvent:
		return "YieldEvent";
	case PauseEvent:
		return "PauseEvent";
	}
	return "unknown";
}

const char* Activity::GetErrorTypeName(ErrorType type)
{
	switch(type) {
	case UnknownError:
		return "UnknownError";
	case AdoptError:
		return "AdoptError";
	case CreateError:
		return "StartError";
	case CompleteError:
		return "CompleteError";
	}
	return "unknown";
}

bool Activity::IsActive() const
{
	return m_state == s_Active;
}

bool Activity::IsStarting() const
{
	switch(m_state) {
	case s_AdoptPending:
	case s_CreatePending:
	case s_Waiting:
		return true;
	case s_NeedsCreation:
	case s_NeedsAdoption:
	case s_Active:
	case s_CompletePending:
	case s_CancelPending:
	case s_Ended:
		return false;
	}

	return false;
}

bool Activity::IsWaitingToStart() const
{
	return m_state == s_Waiting;
}

bool Activity::IsEnding() const
{
	return m_state == s_CompletePending || m_state == s_CancelPending;
}

bool Activity::IsEnded() const
{
	return m_state == s_Ended;
}

bool Activity::CanAdopt() const
{
	return m_state == s_NeedsAdoption
			&& (!m_activityId.undefined() || !m_activityName.empty());
}

bool Activity::CanCreate() const
{
	return m_state == s_NeedsCreation && !m_newActivity.undefined();
}

bool Activity::CanStart() const
{
	return CanAdopt() || CanCreate();
}

void Activity::Create(BusClient& client)
{
	if(m_state != s_NeedsCreation) {
		throw MailException("attempted to create activity in wrong state", __FILE__, __LINE__);
	}

	if(m_newActivity.undefined()) {
		throw MailException("new activity to create not provided", __FILE__, __LINE__);
	}

	MojErr err;

	err = m_newActivity.put("subscribe", true);
	ErrorToException(err);
	err = m_newActivity.put("detailedEvents", true);
	ErrorToException(err);

	client.SendRequest(m_subscriptionSlot, "com.palm.activitymanager", "create", m_newActivity, MojServiceRequest::Unlimited);
	
	m_state = s_CreatePending;
}

void Activity::Adopt(BusClient& client)
{
	if (IsActive())
		throw MailException("can't adopt activity, it is already active", __FILE__, __LINE__);
	else if (!CanAdopt())
		throw MailException("can't adopt activity, not adoptable (no activity id or name available)", __FILE__, __LINE__);
	else
		Adopt(client, m_activityId);
}

void Activity::Adopt(BusClient& client, const MojObject& activityId)
{
	if (m_state != s_NeedsAdoption)
		throw MailException("state is not NeedsAdoption", __FILE__, __LINE__);

	MojObject payload;
	MojErr err = payload.put("activityId", activityId);
	ErrorToException(err);
	err = payload.put("detailedEvents", true);
	ErrorToException(err);
	err = payload.put("subscribe", true);
	ErrorToException(err);
	err = payload.put("wait", true);
	ErrorToException(err);

	client.SendRequest(m_subscriptionSlot, "com.palm.activitymanager", "adopt", payload, MojServiceRequest::Unlimited);

	m_state = s_AdoptPending;
}

void Activity::Start(BusClient& client)
{
	if(CanAdopt()) {
		Adopt(client);
	} else if(CanCreate()) {
		Create(client);
	}
}

MojErr Activity::CreateOrAdoptResponse(MojObject& response, MojErr err)
{
	try {
		MojLogDebug(s_log, "activity %s update: %s", Describe().c_str(), AsJsonString(response).c_str());

		// Update recorded activityInfo, if present
		response.get("$activity", m_activityInfo);

		if(m_state == s_CreatePending) {
			// Check error
			ResponseToException(response, err);

			// Creation response
			err = response.getRequired("activityId", m_activityId);
			ErrorToException(err);

			MojLogInfo(s_log, "created activity %s", Describe().c_str());

			m_state = s_Waiting;
			
			// Don't update with first response, it only contains the new activity ID, a start
			// event will come in its own time.
		} else if(m_state == s_AdoptPending) {
			// Check error
			ResponseToException(response, err);

			bool adopted = false;
			if(response.get("adopted", adopted)) {
				MojLogInfo(s_log, "adopted activity %s", Describe().c_str());

				m_isParent = adopted;

				m_state = s_Active;

				// Implicit start event
				m_updateSignal.call(this, StartEvent);
			} else {
				HandleUpdate(response);
			}
		} else if(m_state == s_Active || m_state == s_Waiting) {
			// Subscription update
			HandleUpdate(response);
		}
	} catch(const std::exception& e) {
		ReportError(GetErrorForState(m_state), e);
	}
	
	return MojErrNone;
}

void Activity::HandleUpdate(MojObject& response)
{
	MojString event;
	MojErr err = response.getRequired("event", event);
	if (err) {
		m_errorSignal.call(this, UnknownError, MojErrException(err, __FILE__, __LINE__));
		return;
	}

	if (event == "start") {
		if(m_state == s_Waiting) {
			m_state = s_Active;
		}

		m_updateSignal.call(this, StartEvent);
	} else if (event == "update") {
		m_updateSignal.call(this, UpdateEvent);
	} else if (event == "orphan") {
		m_updateSignal.call(this, OrphanEvent);
	} else if (event == "cancel") {
		m_subscriptionSlot.cancel();

		m_updateSignal.call(this, CancelEvent);
	}
}

void Activity::Complete(BusClient& client)
{
	if(m_state == s_Active) {
		MojObject payload;
		MojErr err = payload.put("activityId", m_activityId);
		ErrorToException(err);

		client.SendRequest(m_completedSlot, "com.palm.activitymanager", "complete", payload);
		m_state = s_CompletePending;
	} else {
		throw MailException("can not complete an activity that is not active", __FILE__, __LINE__);
	}
}

void Activity::UpdateAndComplete(BusClient& client, const MojObject& updatedFields)
{
	MojLogInfo(s_log, "updating and completing activity %s", Describe().c_str());

	if(m_state == s_Active) {
		MojErr err;
		MojObject payload = updatedFields;

		err = payload.put("activityId", m_activityId);
		ErrorToException(err);
		err = payload.put("restart", true);
		ErrorToException(err);

		client.SendRequest(m_completedSlot, "com.palm.activitymanager", "complete", payload);
		m_state = s_CompletePending;
	} else {
		throw MailException("can not complete an activity that is not active", __FILE__, __LINE__);
	}
}

void Activity::Replace(BusClient& client, const MojObject& updatedFields)
{
	if(IsActive()) {
		UpdateAndComplete(client, updatedFields);
	} else {
		MojErr err;
		MojObject payload;

		err = payload.put("activity", updatedFields);
		ErrorToException(err);
		err = payload.put("start", true);
		ErrorToException(err);
		err = payload.put("replace", true);
		ErrorToException(err);

		m_state = s_CompletePending;
		client.SendRequest(m_replaceSlot, "com.palm.activitymanager", "create", payload);
	}
}

MojErr Activity::ReplaceResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);
		m_state = s_Ended;

		// Update recorded activityInfo, if present
		response.get("$activity", m_activityInfo);

		m_updateSignal.fire(this, CompleteEvent);
	} catch(const std::exception& e) {
		ReportError(CompleteError, e);
	}

	// Consider ended even if it failed
	m_state = s_Ended;

	return MojErrNone;
}

MojErr Activity::CompleteResponse(MojObject& response, MojErr err)
{
	assert(m_state == s_CompletePending);
	
	MojLogInfo(s_log, "activity %s completed", Describe().c_str());

	try {
		// If we try to complete an activity that we aren't the creator of
		// (like an application activity), we'll get a MojErrAccessDenied
		// which we should just ignore for now.
		if (err && err != MojErrAccessDenied)
			ResponseToException(response, err);

		// Cancel the activity manager subscription to remove the activity
		m_subscriptionSlot.cancel();
		m_state = s_Ended;

		// Update recorded activityInfo, if present
		response.get("$activity", m_activityInfo);

		m_updateSignal.fire(this, CompleteEvent);
	} catch(const std::exception& e) {
		ReportError(CompleteError, e);
	}
	
	return MojErrNone;
}

void Activity::Cancel(BusClient& client)
{
	MojObject payload;

	PutIdOrName(payload);

	client.SendRequest(m_cancelSlot, "com.palm.activitymanager", "cancel", payload);
}

MojErr Activity::CancelResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		// Update recorded activityInfo, if present
		response.get("$activity", m_activityInfo);

	} catch(const std::exception& e) {
		// Ignore cancel errors
	}

	m_subscriptionSlot.cancel();

	m_state = s_Ended;
	m_updateSignal.fire(this, CompleteEvent);

	return MojErrNone;
}

void Activity::Unsubscribe()
{
	m_subscriptionSlot.cancel();
}

void Activity::SetEndAction(EndAction action)
{
	m_endAction = action;
}

void Activity::SetUpdatePayload(const MojObject& updatedFields)
{
	m_updatePayload = updatedFields;
}

const char* Activity::GetEndActionName(EndAction endAction)
{
	switch(endAction) {
	case EndAction_Complete: return "complete";
	case EndAction_Restart: return "restart";
	case EndAction_Cancel: return "cancel";
	case EndAction_Unsubscribe: return "unsubscribe";
	}

	return "unknown";
}

const char* Activity::GetEndOrderName(EndOrder endOrder)
{
	switch(endOrder) {
	case EndOrder_First: return "first";
	case EndOrder_Default: return "default";
	case EndOrder_Last: return "last";
	}

	return "unknown";
}

void Activity::End(BusClient& busClient)
{
	MojLogInfo(s_log, "ending (%s) %s", GetEndActionName(m_endAction), Describe().c_str());

	switch(m_endAction) {
	case EndAction_Complete:
		if(IsActive())
			Complete(busClient);
		else
			Cancel(busClient);
		break;
	case EndAction_Restart:
		Replace(busClient, m_updatePayload);
		break;
	case EndAction_Cancel:
		Cancel(busClient);
		break;
	case EndAction_Unsubscribe:
		m_subscriptionSlot.cancel(); // cancel subscription
		m_state = s_Ended;
		m_updateSignal.fire(this, CompleteEvent);
		break;
	}
}

void Activity::ReportError(ErrorType type, const std::exception& exc)
{
	// End first, in case callback checks state
	m_state = s_Ended;
	
	m_errorSignal.call(this, type, exc);

	// TODO: use an error state instead?
	m_subscriptionSlot.cancel();
}

Activity::ErrorType Activity::GetErrorForState(StateType state)
{
	switch(state) {
	case s_NeedsCreation:
	case s_CreatePending:
		return CreateError;
	case s_NeedsAdoption:
	case s_AdoptPending:
		return AdoptError;
	case s_CompletePending:
		return CompleteError;
	default:
		return UnknownError;
	}
}

const char* Activity::GetStateName(StateType state)
{
	switch(state) {
	case s_NeedsCreation:
		return "NeedsCreation";
	case s_NeedsAdoption:
		return "NeedsAdoption";
	case s_AdoptPending:
		return "AdoptPending";
	case s_Waiting:
		return "Waiting";
	case s_Active:
		return "Active";
	case s_CreatePending:
		return "CreatePending";
	case s_CompletePending:
		return "CompletePending";
	case s_CancelPending:
		return "CancelPending";
	case s_Ended:
		return "Ended";
	}

	return "Unknown";
}

void Activity::Status(MojObject& status) const
{
	MojErr err;

	err = status.putString("state", GetStateName(m_state));
	ErrorToException(err);
	err = status.put("activityId", m_activityId);
	ErrorToException(err);

	if(!m_activityName.empty()) {
		err = status.put("activityName", m_activityName);
		ErrorToException(err);
	}
}

void Activity::SetPurpose(ActivityPurpose purpose)
{
	m_purpose = purpose;
}

std::string Activity::Describe() const
{
	std::stringstream ss;
	ss << "<Activity";

	if(!m_activityId.undefined())
		ss << " id=" << AsJsonString(m_activityId);

	if(!m_activityName.empty())
		ss << " name=" << AsJsonString(m_activityName);

	if(m_state != s_CreatePending && m_state != s_AdoptPending)
		ss << " state=" << GetStateName(m_state);

	ss << ">";
	return ss.str();
}
