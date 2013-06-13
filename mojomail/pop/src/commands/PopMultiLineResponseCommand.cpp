// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "commands/PopMultiLineResponseCommand.h"
#include "commands/PopProtocolCommand.h"
#include "PopConfig.h"

const char* const PopMultiLineResponseCommand::TERMINATOR_CHARS = ".";

PopMultiLineResponseCommand::PopMultiLineResponseCommand(PopSession& session)
: PopProtocolCommand(session),
  m_handleEndOfResponse(false),
  m_responsePaused(false),
  m_complete(false),
  m_terminationLine(TERMINATOR_CHARS)
{
	if (m_includesCRLF) {
		m_terminationLine.append(CRLF);
	}
}

PopMultiLineResponseCommand::~PopMultiLineResponseCommand()
{

}

MojErr PopMultiLineResponseCommand::ReceiveResponse()
{
	LineReaderPtr readerPtr = m_session.GetLineReader();
	std::string response;

	try {
		do {
			response = readerPtr->ReadLine(m_includesCRLF);

			if (m_responseFirstLine.empty()) {
				MojLogDebug(m_log, "Command Response: %s", response.c_str());
				m_responseFirstLine = response;
				ParseResponseFirstLine();

				if (m_errorCode == MailError::BAD_USERNAME_OR_PASSWORD
						|| m_errorCode == MailError::ACCOUNT_LOCKED
						|| m_errorCode == MailError::ACCOUNT_UNAVAILABLE
						|| m_errorCode == MailError::ACCOUNT_UNKNOWN_AUTH_ERROR
						|| m_errorCode == MailError::ACCOUNT_WEB_LOGIN_REQUIRED) {
					m_session.LoginFailure(m_errorCode, m_serverMessage);
					Failure(MailException("Login failure", __FILE__, __LINE__));

					return MojErrInternal;
				} else if (m_status == PopProtocolCommand::Status_Err) {
					MojString errMsg;
					MojErr err = errMsg.format("Running command '%s' yields a server error response: '%s'",
							m_requestStr.c_str(), m_serverMessage.c_str());
					ErrorToException(err);

					// finish this command and return an internal error.
					Failure(MailException(errMsg.data(), __FILE__, __LINE__));

					return MojErrInternal;
				}
			} else {
				bool endOfResponse = isEndOfResponse(response);
				if (!endOfResponse || m_handleEndOfResponse) {
					MojLogDebug(m_log, "**** line: '%s'", response.c_str());
					HandleResponse(response);
				}

				if (endOfResponse && !m_handleEndOfResponse) {
					MojLogDebug(m_log, "**** Found command terminator line: '%s'", response.c_str());
					Complete();

					return MojErrNone;
				}
			}
		} while (readerPtr->MoreLinesInBuffer() && !m_responsePaused);

		// wait for more line to read
		if (!m_responsePaused && !readerPtr->Waiting()) {
			readerPtr->WaitForLine(m_handleResponseSlot, PopConfig::READ_TIMEOUT_IN_SECONDS);
		}
	} catch (const MailNetworkTimeoutException& nex) {
		m_errorCode = MailError::CONNECTION_TIMED_OUT;
		NetworkFailure(MailError::CONNECTION_TIMED_OUT, nex);
	} catch (const MailNetworkDisconnectionException& nex) {
		m_errorCode = MailError::NO_NETWORK;
		NetworkFailure(MailError::NO_NETWORK, nex);
	} catch (const MailException& ex) {
		AnalyzeMailException(ex);
	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in receiving multilines response: '%s'", e.what());
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in receiving multilines response");
		Failure(MailException("Unknown exception in receiving multilines response", __FILE__, __LINE__));
	}

	return MojErrNone;
}

void PopMultiLineResponseCommand::PauseResponse()
{
	if (!m_responsePaused) {
		m_responsePaused = true;
		MojLogDebug(m_log, "pausing handling response");
	}
}

void PopMultiLineResponseCommand::ResumeResponse()
{
	if (m_responsePaused) {
		m_responsePaused = false;

		LineReaderPtr readerPtr = m_session.GetLineReader();
		if (readerPtr.get() && !readerPtr->Waiting()) {
			MojLogDebug(m_log, "resuming waiting for response");
			readerPtr->WaitForLine(m_handleResponseSlot, PopConfig::READ_TIMEOUT_IN_SECONDS);
		}
	}
}

void PopMultiLineResponseCommand::Complete()
{
	m_responsePaused = true;

	PopProtocolCommand::Complete();
}

bool PopMultiLineResponseCommand::isEndOfResponse(std::string line)
{
	return line == m_terminationLine;
}
