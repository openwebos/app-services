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

#include "commands/DownloadEmailPartsCommand.h"
#include "commands/FetchEmailCommand.h"
#include "data/EmailSchema.h"
#include "data/UidMap.h"
#include "exceptions/MailException.h"
#include "exceptions/ExceptionUtils.h"
#include "PopDefs.h"

FetchEmailCommand::FetchEmailCommand(PopSession& session, Request::RequestPtr request)
: PopSessionPowerCommand(session, "Fetch email"),
  m_request(request),
  m_email(new PopEmail()),
  m_loadEmailResponseSlot(this, &FetchEmailCommand::LoadEmailResponse),
  m_clearPartsResponseSlot(this, &FetchEmailCommand::ClearPreviousPartsResponse),
  m_getEmailBodyResponseSlot(this, &FetchEmailCommand::GetEmailBodyResponse),
  m_updateEmailSummaryResponseSlot(this, &FetchEmailCommand::UpdateEmailSummaryResponse),
  m_updateEmailPartsResponseSlot(this, &FetchEmailCommand::UpdateEmailPartsResponse)
{
}

FetchEmailCommand::~FetchEmailCommand()
{
}

void FetchEmailCommand::RunImpl()
{
	if (m_session.GetSyncSession().get() && m_session.GetSyncSession()->IsActive()) {
		m_session.GetSyncSession()->RegisterCommand(this);
	}
	LoadEmail();
}

void FetchEmailCommand::LoadEmail()
{
	try {
		m_loadEmailResponseSlot.cancel();
		m_session.GetDatabaseInterface().GetEmail(m_loadEmailResponseSlot, m_request->GetEmailId());
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unable to load email", __FILE__, __LINE__));
	}
}

MojErr FetchEmailCommand::LoadEmailResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojObject results;
		err = response.getRequired("results", results);
		ErrorToException(err);

		MojObject::ArrayIterator itr;
		err = results.arrayBegin(itr);
		ErrorToException(err);

		if (itr == results.arrayEnd()) {
			// no need to fetch email since email is not found
			Complete();
			return MojErrNone;
		} else {
			PopEmailAdapter::ParseDatabasePopObject(*itr, *m_email);
		}

		if (m_request->GetPriority() == Request::Priority_Low && m_email->IsDownloaded()) {
			// this is an auto download and it will not download an email that has been
			// downloaded by an on demand download.
			Complete();
			return MojErrNone;
		} else if (m_request->GetPriority() == Request::Priority_High && m_email->IsDownloaded()) {
			ClearPreviousParts();
			return MojErrNone;
		}

		FetchEmailBody();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Error in loading email response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void FetchEmailCommand::ClearPreviousParts()
{
	try {
		EmailPartList emptyParts;
		m_email->SetPartList(emptyParts);
		m_email->SetDownloaded(false);

		MojObject mojEmail;
		PopEmailAdapter::SerializeToDatabasePopObject(*m_email, mojEmail);
		m_session.GetDatabaseInterface().UpdateItem(m_clearPartsResponseSlot, mojEmail);
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Error in clearing email parts", __FILE__, __LINE__));
	}
}

MojErr FetchEmailCommand::ClearPreviousPartsResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);
		FetchEmailBody();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Error in clearing email parts response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void FetchEmailCommand::FetchEmailBody()
{
	try {
		std::string uid = m_email->GetServerUID();
		boost::shared_ptr<UidMap> uidMap = m_session.GetUidMap();
		int msgNum = uidMap->GetMessageNumber(uid);
		int msgSize = uidMap->GetMessageSize(uid);

		m_downloadBodyResult.reset(new PopCommandResult(m_getEmailBodyResponseSlot));

		MojLogInfo(m_log, "Downloading email body using message number %d with uid '%s'", msgNum, uid.c_str());
		m_downloadBodyCommand.reset(new DownloadEmailPartsCommand(m_session,
				msgNum, msgSize, m_email,
				m_session.GetFileCacheClient(),
				m_request));
		m_downloadBodyCommand->SetResult(m_downloadBodyResult);
		m_downloadBodyCommand->Run();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unable to fetch email body", __FILE__, __LINE__));
	}
}

MojErr FetchEmailCommand::GetEmailBodyResponse()
{
	MojLogInfo(m_log, "Got email body response from server");
	try {
		m_downloadBodyCommand->GetResult()->CheckException();
		UpdateEmailSummary(m_email);
	} catch (const MailFileCacheNotCreatedException& fex) {
		MojLogError(m_log, "Unable to get filecache for the mail: %s", fex.what());
		m_session.Reconnect();
		UpdateEmailParts(m_email);
	} catch (const std::exception& ex) {
		MojLogError(m_log, "Unable to get email body: %s", ex.what());
		UpdateEmailParts(m_email);
	} catch (...) {
		MojLogError(m_log, "Unable to get email body due to unknown error");
		UpdateEmailParts(m_email);
	}

	return MojErrNone;
}

void FetchEmailCommand::UpdateEmailSummary(const PopEmail::PopEmailPtr& emailPtr)
{
	try {
		if (!emailPtr.get()) {
			// this should not happen
			Failure(MailException("Unable to update parts: mail pointer is invalid", __FILE__, __LINE__));
			return;
		}

		MojString m_summary;
		m_summary.append(emailPtr->GetPreviewText().c_str());
		MojLogDebug(m_log, "Saving email with summary"); //: '%s'", emailPtr->GetPreviewText().c_str());
		m_updateEmailSummaryResponseSlot.cancel();
		m_session.GetDatabaseInterface().UpdateEmailSummary(m_updateEmailSummaryResponseSlot, emailPtr->GetId(), m_summary);
	} catch (const std::exception& ex) {
		MojLogError(m_log, "Unable to persist email preview text: %s", ex.what());
		UpdateEmailParts(m_email);
	} catch (...) {
		MojLogError(m_log, "Unable to update email preview text due to unknown error");
		UpdateEmailParts(m_email);
	}
}

MojErr FetchEmailCommand::UpdateEmailSummaryResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);
	} catch (const std::exception& ex) {
		MojLogError(m_log, "Unable to persist email preview text: %s", ex.what());
	} catch (...) {
		MojLogError(m_log, "Unable to update email preview text due to unknown error");
	}

	UpdateEmailParts(m_email);
	return MojErrNone;
}

void FetchEmailCommand::UpdateEmailParts(const PopEmail::PopEmailPtr& emailPtr)
{
	try {
		if (!emailPtr.get()) {
			// this should not happen
			Failure(MailException("Unable to update parts: mail pointer is invalid", __FILE__, __LINE__));
			return;
		}

		MojObject mojEmail;
		MojLogDebug(m_log, "Serializing pop email '%s' parts list", emailPtr->GetServerUID().c_str());
		PopEmailAdapter::SerializeToDatabasePopObject(*emailPtr, mojEmail);

		MojObject m_parts;
		MojErr err = mojEmail.getRequired(EmailSchema::PARTS, m_parts);
		ErrorToException(err);

		MojLogInfo(m_log, "Saving parts of email '%s' to database", AsJsonString(emailPtr->GetId()).c_str());
		m_updateEmailPartsResponseSlot.cancel();
		m_session.GetDatabaseInterface().UpdateEmailParts(m_updateEmailPartsResponseSlot, emailPtr->GetId(), m_parts, !emailPtr->IsDownloaded());
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unable to update email parts", __FILE__, __LINE__));
	}
}

MojErr FetchEmailCommand::UpdateEmailPartsResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		m_request->Done();
		Complete();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unable to update email parts", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void FetchEmailCommand::Complete()
{
	if (m_session.GetSyncSession().get() && m_session.GetSyncSession()->IsActive()) {
		m_session.GetSyncSession()->CommandCompleted(this);
	}

	PopSessionPowerCommand::Complete();
}

void FetchEmailCommand::Failure(const std::exception& ex)
{
	if (m_session.GetSyncSession().get() && m_session.GetSyncSession()->IsActive()) {
		m_session.GetSyncSession()->CommandFailed(this, ex);
	}

	PopSessionPowerCommand::Failure(ex);
}

void FetchEmailCommand::Status(MojObject& status) const
{
	MojErr err;
	PopSessionPowerCommand::Status(status);

	if (m_downloadBodyCommand.get()) {
		MojObject downloadBodyStatus;
		m_downloadBodyCommand->Status(downloadBodyStatus);
		err = status.put("downloadBodyCommand", downloadBodyStatus);
	}
}
