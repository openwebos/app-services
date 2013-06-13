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

#ifndef SMTPSENDMAILCOMMAND_H_
#define SMTPSENDMAILCOMMAND_H_

#include <boost/shared_ptr.hpp>

#include "core/MojSignal.h"
#include "core/MojRefCount.h"
#include "db/MojDbClient.h"

#include "commands/SmtpProtocolCommand.h"
#include "client/SmtpSession.h"

#include "CommonDefs.h"
#include "data/Email.h"
#include "email/AsyncEmailWriter.h"
#include "stream/ByteBufferOutputStream.h"
#include "stream/CRLFTerminatedOutputStream.h"

#include "SmtpClient.h"

class CounterOutputStream;
class AsyncFlowControl;

class SmtpSendMailCommand : public SmtpProtocolCommand
{
public:
	typedef MojSignal<SmtpSession::SmtpError> DoneSignal;
	
	SmtpSendMailCommand(SmtpSession& session,
			   const MojObject& emailId);
	virtual ~SmtpSendMailCommand();
	
	virtual void RunImpl();
	
	void SetSlots(DoneSignal::SlotRef doneSlot);
	
	void Status(MojObject& status) const;

protected:
	static const char* const FROM_COMMAND_STRING;
	static const char* const TO_COMMAND_STRING;
	static const char* const DATA_COMMAND_STRING;
	static const char* const TERMINATE_BODY_STRING;
 	static const char* const RESET_COMMAND_STRING;
 
	struct SendStatus
	{
		enum ErrorType { NoError, Exception, SmtpError } status;
		
		std::exception	exception;
		std::string		smtpErrorText;
		
		SendStatus() : status(NoError), smtpErrorText() {}
	};
	
	void	GetEmail();
	MojErr	GetEmailResponse(MojObject& response, MojErr err);
	
	void	CalculateEmailSize();
	MojErr	FinishCalculateEmailSize(const std::exception* e);
	
	void	WriteEmail();
	MojErr	FinishWriteEmail(const std::exception* e);
	
	void	Send();
	void	SendDone();
	
	void	DeleteEmail();
	MojErr	DeleteEmailResponse(MojObject& response, MojErr err);
	
	// Error handling. Updates send status.
	void	HandleSmtpError(SmtpSession::SmtpError e);
	void	HandleException(const std::exception& e, const char *, const char *, int);
	void	HandleFailure(const std::string& m);
	
	void	UpdateSendStatus();
	MojErr	UpdateSendStatusResponse(MojObject& response, MojErr err);
	
	virtual MojErr HandleResponse(const std::string&);
	
	/**
	 * Queues another SmtpSyncOutbox to send any remaining emails.
	 * 
	 * Both success and error cases should call this after everything is done.
	 */
	void	QueueSyncOutbox();
	
	/**
	 * Reports status back to SmtpClient
	 */
	void	Done();
	
	MojObject	m_emailId;
	Email		m_email;
	
	SendStatus	m_sendStatus;
	
	MojRefCountedPtr<AsyncEmailWriter>			m_emailWriter; // signal handlers must be refcounted
	MojRefCountedPtr<CounterOutputStream>		m_counter;
	MojRefCountedPtr<CRLFTerminatedOutputStream>	m_crlfTerminator;
	size_t										m_bytesLeft;
	
	MojRefCountedPtr<AsyncFlowControl>			m_flowControl;

	// Database slots
	MojDbClient::Signal::Slot<SmtpSendMailCommand> m_getEmailSlot;
	MojDbClient::Signal::Slot<SmtpSendMailCommand> m_updateSendStatusSlot;
	
	// AsyncEmailWriter slots
	AsyncEmailWriter::EmailWrittenSignal::Slot<SmtpSendMailCommand> m_calculateDoneSlot;
	AsyncEmailWriter::EmailWrittenSignal::Slot<SmtpSendMailCommand> m_writeDoneSlot;
	
	DoneSignal	m_doneSignal;
	
	enum {
		State_SendMailFrom,
		State_SendRcptTo,
		State_SendData,
		State_SendBody,
		State_SendRset,
		State_SendErrorRset
	}	m_write_state;
	unsigned int		m_toIdx;
	std::vector<std::string>	m_toAddress;
	SmtpSession::SmtpError	m_error;
};

#endif /*SMTPSENDMAILCOMMAND_H_*/
