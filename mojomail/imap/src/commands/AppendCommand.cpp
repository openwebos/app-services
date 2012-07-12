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

#include "commands/AppendCommand.h"
#include "client/ImapSession.h"
#include "ImapPrivate.h"

// Time to wait for the email to finish writing
const int AppendCommand::APPEND_WRITE_COMPLETE_TIMEOUT = 5 * 60; // 5 minutes

// Time to wait for OK response
const int AppendCommand::APPEND_RESPONSE_TIMEOUT = 2 * 60; // 2 minutes

const char* const AppendCommand::APPEND_TERMINATION_STRING = "\r\n";

AppendCommand::AppendCommand(ImapSession& session, const MojObject& folderId, const EmailPtr& emailP, const std::string& flags, const std::string& folderName)
: ImapSyncSessionCommand(session, folderId),
  m_bytesLeft(0),
  m_emailP(emailP),
  m_flags(flags),
  m_folderName(folderName),
  m_uid(0),
  m_uidValidity(0),
  m_calculateDoneSlot(this, &AppendCommand::FinishCalculateEmailSize),
  m_appendToServerContinueSlot(this, &AppendCommand::HandleAppendContinueResponse),
  m_writeDoneSlot(this, &AppendCommand::FinishWriteEmail),
  m_appendToServerDoneSlot(this, &AppendCommand::HandleAppendDoneResponse)
{
}

AppendCommand::~AppendCommand()
{
}

void AppendCommand::RunImpl()
{
	CalculateEmailSize();
}


void AppendCommand::CalculateEmailSize()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "calculating email size");

	m_counter.reset( new CounterOutputStream() );
	m_emailWriter.reset( new AsyncEmailWriter(*m_emailP) );
	m_emailWriter->SetOutputStream(m_counter);
	m_emailWriter->SetPartList(m_emailP->GetPartList());
	m_emailWriter->WriteEmail(m_calculateDoneSlot);
}

MojErr AppendCommand::FinishCalculateEmailSize(const std::exception* exc)
{
	CommandTraceFunction();

	if(exc) {
		Failure(*exc);
		return MojErrNone;
	}

	try {
		m_bytesLeft = m_counter->GetBytesWritten();
		MojLogInfo(m_log, "done calculating email size: %d bytes", m_bytesLeft);

		SendAppendRequest();

	} CATCH_AS_FAILURE

	return MojErrNone;
}


void AppendCommand::SendAppendRequest()
{
	MojLogInfo(m_log, "sending APPEND command");

	stringstream commandStr;
	commandStr << "APPEND " << QuoteString(m_folderName.data());

	if(!m_flags.empty()) {
		commandStr << " (" << m_flags << ")";
	}
	commandStr << " {" << m_bytesLeft << "}";

	m_appendToServerResponseParser.reset(new AppendResponseParser(m_session, m_appendToServerDoneSlot));
	m_appendToServerResponseParser->SetContinuationResponseSlot(m_appendToServerContinueSlot);

	m_session.SendRequest(commandStr.str(), m_appendToServerResponseParser);
}

MojErr AppendCommand::HandleAppendContinueResponse()
{
	MojLogInfo(m_log, "IMAP APPEND command + ready for literal data");

	try {
		WriteEmail();

		// Max time we'll spend trying to write the email out
		// FIXME should reset timer when data is sent
		m_session.UpdateRequestTimeout(m_appendToServerResponseParser, APPEND_WRITE_COMPLETE_TIMEOUT);
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void AppendCommand::WriteEmail()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "Appending literal");
	assert(m_emailWriter.get());
	m_emailWriter->SetOutputStream(m_session.GetOutputStream());
	m_emailWriter->WriteEmail(m_writeDoneSlot);
}

MojErr AppendCommand::FinishWriteEmail(const std::exception* exc)
{
	CommandTraceFunction();

	if(exc) {
		Failure(*exc);
		return MojErrNone;
	} else {
		try {
			OutputStreamPtr outputStream = m_session.GetOutputStream();
			outputStream->Write(APPEND_TERMINATION_STRING);

			// Flush the buffer -- this is important
			outputStream->Flush();

			// Now we'll wait for the server to respond
			m_session.UpdateRequestTimeout(m_appendToServerResponseParser, APPEND_RESPONSE_TIMEOUT);
		} CATCH_AS_FAILURE
	}

	return MojErrNone;
}


MojErr AppendCommand::HandleAppendDoneResponse()
{
	CommandTraceFunction();

	MojLogInfo(m_log, "IMAP APPEND request done");

	try {
		m_appendToServerResponseParser->CheckStatus();

		if(m_session.GetCapabilities().HasCapability(Capabilities::UIDPLUS))
		{
			m_uid = m_appendToServerResponseParser->GetUid();
			m_uidValidity = m_appendToServerResponseParser->GetUidVaildity();
		}

		Complete();
	} CATCH_AS_FAILURE

	return MojErrNone;
}
