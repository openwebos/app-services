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

#include "commands/SmtpSendMailCommand.h"
#include "exceptions/MailException.h"
#include "stream/CounterOutputStream.h"
#include "stream/CRLFTerminatedOutputStream.h"
#include "data/EmailAddress.h"
#include "data/EmailPart.h"
#include "data/EmailAdapter.h"
#include "data/EmailSchema.h"
#include "boost/lexical_cast.hpp"
#include "async/AsyncOutputStream.h"
#include "async/AsyncFlowControl.h"

#define HandleUnknownException() \
	HandleException(MailException("unknown exception", __FILE__, __LINE__), __func__, __FILE__, __LINE__)

SmtpSendMailCommand::SmtpSendMailCommand(SmtpSession& session, const MojObject& emailId)
: SmtpProtocolCommand(session),
	m_emailId(emailId),
	m_bytesLeft(0),
	m_getEmailSlot(this, &SmtpSendMailCommand::GetEmailResponse),
	m_updateSendStatusSlot(this, &SmtpSendMailCommand::UpdateSendStatusResponse),
	m_calculateDoneSlot(this, &SmtpSendMailCommand::FinishCalculateEmailSize),
	m_writeDoneSlot(this, &SmtpSendMailCommand::FinishWriteEmail),
	m_doneSignal(this)
{
	printf("Construction of SmtpSendMailCommand %p\n", this);
}

SmtpSendMailCommand::~SmtpSendMailCommand()
{
	printf("Destruction of SmtpSendMailCommand %p\n", this);
}

void SmtpSendMailCommand::SetSlots(DoneSignal::SlotRef doneSlot)
{
	m_doneSignal.connect(doneSlot);
}

void SmtpSendMailCommand::RunImpl()
{
	try {
		if (m_session.HasError()) {
			if (m_session.GetError().errorOnEmail) {
				// this doesn't make any sense, but if we don't mark the mail as bad, someone
				// is going to loop...
				m_error = m_session.GetError();
				MojLogInfo(m_log, "SendMail dying off, marking mail as bad ...");
				UpdateSendStatus();
			} else {
				MojLogInfo(m_log, "SendMail dying off, due to session error state ...");
				m_doneSignal.fire(m_session.GetError());
				Complete();
			}
			return;
		}

		MojString json;
		m_emailId.toJson(json);
		MojLogInfo(m_log, "running sendMail id=%s", json.data());
	
		GetEmail();

	} catch (const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch (...) {
		HandleUnknownException();
	}
}

void SmtpSendMailCommand::GetEmail()
{
	try {
		// Get the email from the database
		MojLogInfo(m_log, "trying GetOutboxEmail query");

		m_session.GetDatabaseInterface().GetOutboxEmail(m_getEmailSlot, m_emailId);

		MojLogInfo(m_log, "started GetOutboxEmail query");

	} catch (const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch (...) {
		HandleUnknownException();
	}
}

MojErr SmtpSendMailCommand::GetEmailResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		MojString json;
		response.toJson(json);
		MojLogInfo(m_log, "got Email response");
		MojLogDebug(m_log, "Response: %s", json.data());

		// check the database result first
		ErrorToException(err);

		MojObject results;
		err = response.getRequired(_T("results"), results);
		ErrorToException(err);
		
		if(results.size() == 0) {
			throw MailException("email not found", __FILE__, __LINE__);
		}
		
		// There should only be one email for a given id
		assert(results.size() == 1);

		MojObject emailObj;
		bool hasObject = results.at(0, emailObj);
		if(!hasObject) {
			throw MailException("error getting email from results", __FILE__, __LINE__);
		}

		EmailAdapter::ParseDatabaseObject(emailObj, m_email);
		
		m_toAddress.clear();
		
		if (m_email.GetTo()) {
			for (EmailAddressList::iterator i = m_email.GetTo()->begin();
				i != m_email.GetTo()->end();
				i++)
				m_toAddress.push_back((*i)->GetAddress());
		}
		if (m_email.GetCc()) {
			for (EmailAddressList::iterator i = m_email.GetCc()->begin();
				i != m_email.GetCc()->end();
				i++)
				m_toAddress.push_back((*i)->GetAddress());
		}
		if (m_email.GetBcc()) {
			for (EmailAddressList::iterator i = m_email.GetBcc()->begin();
				i != m_email.GetBcc()->end();
				i++)
				m_toAddress.push_back((*i)->GetAddress());
		}
		
		m_toIdx = 0;

		// FIXME: Subscribe to file cache to pin cache contents
		
		CalculateEmailSize();
		
	} catch (const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch (...) {
		HandleUnknownException();
	}

	return MojErrNone;
}

void SmtpSendMailCommand::CalculateEmailSize()
{
	try {
		MojLogInfo(m_log, "calculating email size");
		m_counter.reset( new CounterOutputStream() );

		// Wrap the mail writer in a CRLF fixer stream, which ensures that the stream ends with 
		// CRLF, so we can safely write out a DOT to end the body, without the risk that it'll end
		// up on the end of a body line. We're doing that here, so the counter will include those
		// octets.
		m_crlfTerminator.reset( new CRLFTerminatedOutputStream(m_counter));
	
		m_emailWriter.reset( new AsyncEmailWriter(m_email) );
		m_emailWriter->SetOutputStream(m_crlfTerminator );
		m_emailWriter->SetBccIncluded(false); // bcc should only appear in the RCPT list
	
		m_emailWriter->SetPartList(m_email.GetPartList());
	
		m_emailWriter->WriteEmail(m_calculateDoneSlot);

	} catch (const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch (...) {
		HandleUnknownException();
	}
}

MojErr SmtpSendMailCommand::FinishCalculateEmailSize(const std::exception* exc)
{
	if(exc) {
		HandleException(*exc, __func__, __FILE__, __LINE__);
		return MojErrNone;
	}
	
	try {
		m_bytesLeft = m_counter->GetBytesWritten();
		MojLogInfo(m_log, "done calculating email size");
		MojLogDebug(m_log, "email size: %d bytes", m_bytesLeft);

		m_write_state = State_SendMailFrom;

		// If SIZE extension is present, and server specified a fixed maximum size,
		// check calculated mail size against server limit.
		if (m_session.GetSizeMax() && m_session.GetSizeMaxValue() > 0) {
			if (m_bytesLeft > m_session.GetSizeMaxValue()) {
				MojLogInfo(m_log, "Outgoing mail is larger than server stated SIZE, so rejecting immediately");
				m_error.internalError = "Message larger than server stated limit, not trying to send";
				m_error.errorCode = MailError::EMAIL_SIZE_EXCEEDED;
				m_error.errorText = ""; // No server text, cannot present message
				m_error.errorOnEmail = true;
				m_write_state = State_SendErrorRset;
			}
		}

		WriteEmail();
		
	} catch(const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch(...) {
		HandleUnknownException();
	}
	
	return MojErrNone;
}

const char* const SmtpSendMailCommand::FROM_COMMAND_STRING           = "MAIL FROM:";
const char* const SmtpSendMailCommand::TO_COMMAND_STRING             = "RCPT TO:";
const char* const SmtpSendMailCommand::DATA_COMMAND_STRING           = "DATA";
const char* const SmtpSendMailCommand::TERMINATE_BODY_STRING         = ".";
const char* const SmtpSendMailCommand::RESET_COMMAND_STRING          = "RSET";

void SmtpSendMailCommand::WriteEmail()
{
	try {
		switch (m_write_state) {
		case State_SendMailFrom:
		{
			MojLogInfo(m_log, "Sending MAIL FROM command");
			//MojLogInfo(m_log, "From address is %s\n", m_email.GetFrom()->GetAddress().c_str());

			std::string userCommand = std::string(FROM_COMMAND_STRING) + " <" + m_email.GetFrom()->GetAddress() + ">";
		
			// If SIZE extension is present, place message size on FROM line so that the server
			// can potentially reject it now, instead of after we transmit the data.
			if (m_session.GetSizeMax()) {
				char buf[64];
				memset(buf, '\0', sizeof(buf));
				snprintf(buf, sizeof(buf)-1, " SIZE=%d", m_bytesLeft);
				userCommand += buf;

				MojLogInfo(m_log, "...with SIZE");
			}
		
			SendCommand(userCommand, SmtpSession::TIMEOUT_MAIL_FROM);
			break;
		}
		case State_SendRcptTo:
		{
			MojLogInfo(m_log, "Sending RCPT TO command");
			//MojLogInfo(m_log, "To address is %s\n", m_toAddress[m_toIdx].c_str());

			std::string userCommand = std::string(TO_COMMAND_STRING) + " <" + m_toAddress[m_toIdx] + ">";
			SendCommand(userCommand, SmtpSession::TIMEOUT_RCPT_TO);
			break;
		}
		case State_SendData:
		{
			MojLogInfo(m_log, "Sending DATA command");
		
			std::string userCommand = DATA_COMMAND_STRING;
			SendCommand(userCommand, SmtpSession::TIMEOUT_DATA_INITIATION);
			break;
		}
		case State_SendBody:
		{
			MojLogInfo(m_log, "Sending BODY");
		
			assert(m_emailWriter.get());

			OutputStreamPtr outputStream = m_session.GetConnection()->GetOutputStream();
		
			// Wrap the mail writer in a CRLF fixer stream, which ensures that the stream ends with 
			// CRLF, so we can safely write out a DOT to end the body, without the risk that it'll end
			// up on the end of a body line.
			m_crlfTerminator.reset( new CRLFTerminatedOutputStream(outputStream) );

			// connect mail writer
			m_emailWriter->SetOutputStream(m_crlfTerminator);
		
			// FIXME: set up timeout on writing email body blocks
			//m_emailWriter->SetDataTimeout(SmtpSession::TIMEOUT_DATA_BLOCK);

			// Set up flow control
			AsyncOutputStream* asyncOs = dynamic_cast<AsyncOutputStream*>(outputStream.get());
			if(asyncOs) {
				m_flowControl.reset(new AsyncFlowControl(m_emailWriter, asyncOs));
			}
	
			// Send the DOT after the body is written.
			m_emailWriter->WriteEmail(m_writeDoneSlot);
			break;
		}
		case State_SendRset:
		{
			MojLogInfo(m_log, "Sending RSET");
		
			std::string userCommand = RESET_COMMAND_STRING;
			SendCommand(userCommand);
			break;
		}
		case State_SendErrorRset:
		{
			MojLogInfo(m_log, "Sending RSET after error");
		
			std::string userCommand = RESET_COMMAND_STRING;
			SendCommand(userCommand);
			break;
		}
		default:
		{
			MojLogInfo(m_log, "Unknown SmtpSendMailCommand::WriteEmail write mail state %d", m_write_state);
			HandleFailure("bad WriteEmail state");
			break;
		}
		}

	} catch(const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch(...) {
		HandleUnknownException();
	}
}

MojErr SmtpSendMailCommand::HandleResponse(const std::string& line)
{
	try {
		switch (m_write_state) {
		case State_SendMailFrom:
		{
			MojLogInfo(m_log, "MAIL FROM command response");

			if (m_status == Status_Ok) {
				MojLogInfo(m_log, "MAIL FROM command +OK");
				m_toIdx = 0;
				if(m_toIdx < m_toAddress.size()) {
					m_write_state = State_SendRcptTo;
				} else {
					m_error.errorCode = MailError::BAD_RECIPIENTS; // Error out if for some reason there is no recipient.
					m_error.errorOnEmail = true;
					m_error.errorOnAccount = false;
					m_error.internalError = "Error setting forward address";
					m_write_state = State_SendErrorRset;
				}
				WriteEmail();
			} else {
				MojLogInfo(m_log, "MAIL FROM command -ERR");
				m_error = GetStandardError();
				m_error.internalError = "Error setting reverse address";
				m_write_state = State_SendErrorRset;
				WriteEmail();
			}
			break;
		}
		case State_SendRcptTo:
		{
			MojLogInfo(m_log, "RCPT TO command response");
 
			if (m_status == Status_Ok) {
				m_toIdx++;
				if (m_toIdx < m_toAddress.size()) {
					MojLogInfo(m_log, "RCPT TO command +OK, more to come");
					m_write_state = State_SendRcptTo;
				} else {
					MojLogInfo(m_log, "RCPT TO command +OK, done");
					m_write_state = State_SendData;
				}
				WriteEmail();
			
			} else {
				MojLogInfo(m_log, "RCPT TO command -ERR");
				m_error =  GetStandardError();
				if (m_status == 550) { // general failure means bad recipient
					m_error.errorCode = MailError::BAD_RECIPIENTS;
					m_error.errorOnEmail = true;
					m_error.errorOnAccount = false;
				}
				// 452 is technically a non-fatal error that says no more recipients can be
				// added, on the expectation we'll go and send our mail, and then send a
				// copy to the next batch of recipients. Instead, we'll treat this as a 
				// permanent error, and not send it to anyone (by sending RSET, and not DATA).
				// 552 is (as the spec says) an old and occasionally seen typo of 452.
				if (m_status == 452 || m_status == 552) {
					m_error.errorCode = MailError::BAD_RECIPIENTS; // too many, actually
					m_error.errorOnEmail = true;
					m_error.errorOnAccount = false;
				}
				m_error.internalError = "Error setting forward address";
				m_write_state = State_SendErrorRset;
				WriteEmail();
			}
			break;
		}
		case State_SendData:
		{
			MojLogInfo(m_log, "DATA command response");

			if (m_status == Status_Ok) {
				MojLogInfo(m_log, "DATA command +OK");
				m_write_state = State_SendBody;
				WriteEmail();
			
			} else {
				MojLogInfo(m_log, "DATA command -ERR");
				m_error = GetStandardError();
				m_error.internalError = "Error starting mail body";
				m_write_state = State_SendErrorRset;
				WriteEmail();
			}
			break;
		}
		case State_SendBody:
		{
			MojLogInfo(m_log, "BODY command response");

			if (m_status == Status_Ok) {
				MojLogInfo(m_log, "BODY command +OK");
				m_write_state = State_SendRset;
				WriteEmail();
			
			} else {
				MojLogInfo(m_log, "BODY command -ERR");
				m_error = GetStandardError();
				m_error.internalError = "Error completing send";
				m_write_state = State_SendErrorRset;
				WriteEmail();
			}
			break;
		}
		case State_SendRset:
		{
			MojLogInfo(m_log, "RSET command response");

			if (m_status == Status_Ok) {
				MojLogInfo(m_log, "RSET command +OK");
				SendDone();
			} else {
				MojLogInfo(m_log, "RSET command -ERR");
				m_error = GetStandardError();
				m_error.internalError = "Error resetting";
				HandleSmtpError(m_error);
			}
			break;
		}
		case State_SendErrorRset:
		{
			// Handle an error response after clearing an error: if we had one bad e-mail, we want to
			// be sure we are in a good state before going to the next e-mail.
			HandleSmtpError(m_error);
			break;
		}
		default:
		{
			MojLogInfo(m_log, "Unknown write mail state %d in SmtpSendMailCommand::HandleResponse", m_write_state);
			HandleFailure("bad HandleResponse state");
			break;
		}
		}

	} catch(const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch(...) {
		HandleUnknownException();
	}
	
	return MojErrNone;
}

MojErr SmtpSendMailCommand::FinishWriteEmail(const std::exception* exc)
{
	if(exc) {
		HandleException(*exc, __func__, __FILE__, __LINE__);
		return MojErrNone;
	}
	
	try {
		m_crlfTerminator.reset();

		SendCommand(TERMINATE_BODY_STRING, SmtpSession::TIMEOUT_DATA_TERMINATION);

	} catch(const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch(...) {
		HandleUnknownException();
	}
	
	return MojErrNone;
}

void SmtpSendMailCommand::SendDone()
{
	try {
		MojLogInfo(m_log, "Send Done Successfully.  Now update status!");
		UpdateSendStatus();

	} catch(const std::exception& e) {
		HandleException(e, __func__, __FILE__, __LINE__);
	} catch(...) {
		HandleUnknownException();
	}
}


void SmtpSendMailCommand::HandleSmtpError(SmtpSession::SmtpError error)
{
	m_error = error;
	
	UpdateSendStatus();
}

void SmtpSendMailCommand::HandleFailure(const std::string& m)
{
	m_error.errorCode = MailError::INTERNAL_ERROR; 
	m_error.errorText = ""; // no localized text available
	m_error.errorOnEmail = true; // Safest assumption
	m_error.internalError = "Internal failure: " + m;
	
	UpdateSendStatus();
}

void SmtpSendMailCommand::HandleException(const std::exception& e, const char *func, const char * file, int line)
{
	bool isConnectionError = false;

	// Check for error set on connection
	if(m_session.GetConnection().get()) {
		MailError::ErrorInfo errorInfo = m_session.GetConnection()->GetErrorInfo();

		if(errorInfo.errorCode != MailError::NONE) {
			isConnectionError = true;
			m_error.errorCode = errorInfo.errorCode;
			m_error.errorText = errorInfo.errorText;
			m_error.errorOnAccount = true;
			m_error.errorOnEmail = false;
			m_error.internalError = "Connection error";

			fprintf(stderr, "connection error code %d\n", errorInfo.errorCode);
		} else if(m_session.GetConnection()->IsClosed()) {
			isConnectionError = true;
			m_error.errorCode = MailError::CONNECTION_FAILED;
			m_error.errorText = "connection closed";
			m_error.errorOnAccount = true;
			m_error.errorOnEmail = false;
		}
	}

	if(!isConnectionError) {
		m_error.errorCode = MailError::INTERNAL_ERROR;
		m_error.errorText = ""; // no localized text available
		m_error.errorOnEmail = true;	// Safest assumption
		m_error.internalError = std::string("Internal exception in ") + file + " line " + boost::lexical_cast<std::string>(line)  + ": " + e.what();
	}
	
	MojLogInfo(m_log, "Caught exception %s in %s at %s:%d", e.what(), func, file, line);

	UpdateSendStatus();
}

void SmtpSendMailCommand::UpdateSendStatus()
{
	try {
	
		MojObject sendStatus;
		MojObject visible;
		MojErr err;

		bool update = false;

		if (m_error.errorCode == 0) {

			MojLogInfo(m_log, "updating send status to SENT successfully");
			err = sendStatus.put(EmailSchema::SendStatus::SENT, true);
			ErrorToException(err);
			err = visible.put(EmailSchema::Flags::VISIBLE, false);
			ErrorToException(err);
			update = true;

		} else if (m_error.errorOnEmail) {

			MojLogInfo(m_log, "updating send status on email to indicate error");
		
			MojString str;
			str.assign(m_error.errorText.c_str());
			err = sendStatus.put(EmailSchema::SendStatus::LAST_ERROR, str); 
			ErrorToException(err);
			// FIXME: Should we store an actual error code?
			err = sendStatus.put(EmailSchema::SendStatus::FATAL_ERROR, true);
			ErrorToException(err);
			update = true;

		} 
		// account errors are not handled in sent status: message
		// status will not be changed in those cases.
	
		if (update) {
	
			MojString idJson;
			m_emailId.toJson(idJson);
			MojLogInfo(m_log, "updating send status for email %s", idJson.data());
			m_session.GetDatabaseInterface().UpdateSendStatus(m_updateSendStatusSlot, m_emailId, sendStatus, visible);

		} else {
			
			// Finish up now
			Done();
		}
	} catch (std::exception & e) {
		// Error updating status, just give up.
		MojLogInfo(m_log, "Exception %s hit trying to update send status, bailing", e.what());
		Done();
	} catch (...) {
		// Error updating status, just give up.
		MojLogInfo(m_log, "Unknown exception hit trying to update send status, bailing");
		Done();
	}
}

MojErr SmtpSendMailCommand::UpdateSendStatusResponse(MojObject& response, MojErr err)
{
	try {
		// N.B. update failures should probably be non-fatal
		// It might just mean the email was already deleted by someone else
		if (err)
			MojLogInfo(m_log, "Error updating send status: %d", err);
		//ErrorToException(err);

	} catch (std::exception & e) {
		// Error updating status, just give up.
		MojLogInfo(m_log, "Exception %s hit after updating send status, bailing", e.what());
	} catch (...) {
		// Error updating status, just give up.
		MojLogInfo(m_log, "Unknown exception after updating send status, bailing");
	}

	Done();

	return MojErrNone;
}

void SmtpSendMailCommand::Done()
{
	// disconnect all slots (otherwise this object will leak)
	m_getEmailSlot.cancel();
	m_updateSendStatusSlot.cancel();
	m_calculateDoneSlot.cancel();
	m_writeDoneSlot.cancel();

	// Tell the SmtpSyncOutbox command we're done so it can send the next e-mail
	
	// Any errors found will be reported this way.
	m_doneSignal.fire(m_error);
	
	Complete();
}

void SmtpSendMailCommand::Status(MojObject& status) const
{
	MojErr err;
	SmtpProtocolCommand::Status(status);

	err = status.put("emailId", m_emailId);
	ErrorToException(err);

	if(m_counter.get()) {
		err = status.put("emailSize", (MojInt64) m_counter->GetBytesWritten());
		ErrorToException(err);
	}
}
