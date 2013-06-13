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

#include "commands/DownloadEmailHeaderCommand.h"
#include "PopDefs.h"
#include "email/AsyncEmailParser.h"

const char* const DownloadEmailHeaderCommand::COMMAND_STRING		= "TOP";

DownloadEmailHeaderCommand::DownloadEmailHeaderCommand(PopSession& session, int msgNum,
													   const EmailPtr& email,
													   MojSignal1<bool>::SlotRef doneSlot)
: PopMultiLineResponseCommand(session),
  m_msgNum(msgNum),
  m_parseFailed(false),
  m_doneSignal(this)
{
	m_includesCRLF = true;
	m_terminationLine.append(CRLF);
	m_handleEndOfResponse = true;
	m_doneSignal.connect(doneSlot);

	m_emailParser.reset(new AsyncEmailParser());
	m_emailParser->EnableHeaderParsing();
	m_emailParser->SetEmail(email);
}

DownloadEmailHeaderCommand::~DownloadEmailHeaderCommand()
{

}

void DownloadEmailHeaderCommand::RunImpl()
{
	m_emailParser->Begin();

	// Command syntax: "TOP <msg_num> 0".  0 means that we won't get any lines from the body
	std::ostringstream command;
	command << COMMAND_STRING << " " << m_msgNum << " 0";

	SendCommand(command.str());
}

MojErr DownloadEmailHeaderCommand::HandleResponse(const std::string& line)
{
	MojLogDebug(m_log, "*** Header data: '%s'", line.c_str());

	try {
		if (!isEndOfResponse(line)) {
			m_emailParser->ParseLine(line.data(), line.length());
		} else {
			m_emailParser->End();
			Complete();
		}

	} catch (const std::exception& e) {
		MojLogError(m_log, "Encountered error in parsing email header: '%s'", e.what());
		m_parseFailed = true;
	} catch (...) {
		MojLogError(m_log, "Encountered error in parsing email header");
		m_parseFailed = true;
	}

	return MojErrNone;
}

void DownloadEmailHeaderCommand::ParseFailed()
{
	MojLogInfo(m_log, "Encountered error in parsing email header");
	m_parseFailed = true;
}

void DownloadEmailHeaderCommand::Complete()
{
	MojLogInfo(m_log, "Completed download email header command");
	m_doneSignal.fire(m_parseFailed);

	PopMultiLineResponseCommand::Complete();
}

void DownloadEmailHeaderCommand::Cleanup()
{
	m_emailParser.reset();
}

void DownloadEmailHeaderCommand::Failure(const std::exception& exc)
{
	MojLogError(m_log, "Exception in command %s: %s", Describe().c_str(), exc.what());
	MojRefCountedPtr<PopCommandResult> result = GetResult();
	result->SetException(exc);

	Complete();
}
