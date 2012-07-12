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

#include "async/GIOChannelWrapper.h"
#include "ParseEmlHandler.h"
#include "util/StringUtils.h"
#include "data/EmailAdapter.h"
#include "PopClient.h"
#include "sandbox.h"
#include <boost/algorithm/string/predicate.hpp>
#include "email/AsyncEmailParser.h"

using namespace std;

const int ParseEmlHandler::READ_TIMEOUT_IN_SECONDS = 30;

MojLogger& ParseEmlHandler::s_log = PopClient::s_log;

ParseEmlHandler::ParseEmlHandler(PopBusDispatcher* busDispatcher,
							boost::shared_ptr<BusClient> bClient,
							MojRefCountedPtr<CommandManager> manager,
							MojRefCountedPtr<MojServiceMessage> msg,
							string filePath)
: Command(*manager.get(), Command::NormalPriority),
  m_popBusDispatcher(busDispatcher),
  m_bClient(bClient),
  m_fileCacheClient(new FileCacheClient(*m_bClient.get())),
  m_email(new Email()),
  m_msg(msg),
  m_filePath(filePath),
  m_paused(false),
  m_needMoreData(true),
  m_ioFactory(new GIOChannelWrapperFactory()),
  m_emlChannelReadableSlot(this, &ParseEmlHandler::HandleEmlDataAvailable),
  m_parserReadySlot(this, &ParseEmlHandler::ParserReady),
  m_parserDoneSlot(this, &ParseEmlHandler::ParserDone)
{
	MojLogInfo(s_log, "put ParseEmlHandler into the queue.");
	manager->QueueCommand(this);
}

ParseEmlHandler::~ParseEmlHandler()
{
}

void ParseEmlHandler::Run()
{
	RunImpl();
}
void ParseEmlHandler::RunImpl()
{
	MojLogDebug(s_log, "ParseEmlHandler::Run()\n");
	try {
		GetEmlFile();
	} catch(const std::exception& e) {
		ParseEmlFailed(e);
	}
}

void ParseEmlHandler::GetEmlFile()
{
	MojLogDebug(s_log, "ParseEmlHandler::GetEmlFile(): %s\n", m_filePath.c_str());
	if(!m_filePath.empty()) {
		StringUtils::SanitizeFilePath(m_filePath);

		bool isPathAllowed = SBIsPathAllowed(m_filePath.c_str(), "" /* publicly-accessible files only */, SB_READ);

		// FIXME: exempt /tmp until people stop using it
		if(!isPathAllowed) {
			// NOTE: this allocates memory which must be freed.
			char* tempPath = SBCanonicalizePath(m_filePath.c_str());

			std::string canonicalPath;

			if(tempPath) {
				if(tempPath)
					canonicalPath.assign(tempPath);

				free(tempPath);
			}

			// NOTE: this doesn't check for malicious symlinks
			if(boost::starts_with(canonicalPath, "/tmp/")) {
				isPathAllowed = true;
			}
		}

		// Check if the path is valid
		if(!isPathAllowed) {
			MojLogWarning(s_log, "attempted to access part path outside of sandbox");
			throw MailException("invalid part file path: permission denied", __FILE__, __LINE__);
		}

		// Create channel

		try {
			m_emlChannel = m_ioFactory->OpenFile(m_filePath.c_str(), "r");
			m_lineReader = new LineReader(m_emlChannel->GetInputStream());
			m_lineReader->SetNewlineMode(LineReader::NewlineMode_Auto);

			// Setup parser
			m_emailParser.reset(new AsyncEmailParser());
			m_emailParser->SetEmail(m_email);
			m_emailParser->EnableHeaderParsing();
			m_emailParser->EnableBodyParsing(*m_fileCacheClient, 16 * 1024);
			m_emailParser->SetReadySlot(m_parserReadySlot);
			m_emailParser->SetDoneSlot(m_parserDoneSlot);
			m_emailParser->Begin();

			m_lineReader->WaitForLine(m_emlChannelReadableSlot, READ_TIMEOUT_IN_SECONDS);
		} catch(const std::exception& e) {
			MojLogError(s_log, "error opening file %s: %s", m_filePath.c_str(), e.what());
			ParseEmlFailed(e);
		}
	} else {
		// no file path
		throw MailException("EML filepath is empty ", __FILE__, __LINE__);
	}
}

MojErr ParseEmlHandler::HandleEmlDataAvailable()
{
	// NOTE: exceptions will be handled inside ReadPartdata()

	if(!m_paused) {
		MojLogDebug(s_log, "ParseEmlHandler::HandleEmlDataAvailable()\n");
		ReadEmlAndParseData();
	} else {
		MojLogDebug(s_log, "ParseEmlHandler::HandleEmlDataAvailable() waiting for parsing resume\n");
	}

	return MojErrNone;
}


bool ParseEmlHandler::ReadEmlAndParseData()
{
	try {

		while (m_lineReader->MoreLinesInBuffer()) {
			string line = m_lineReader->ReadLine(false);

			// Append (normalized) newline
			line.append("\r\n");

			m_emailParser->ParseLine(line.data(), line.length());

			if (!m_emailParser->IsReadyForData()) {
				m_paused = true;
				break;
			}
		}

		if (m_lineReader->IsEOF()) {
			m_emailParser->End();
		} else {
			if (m_emailParser->IsReadyForData()) {
				m_paused = false;
				m_lineReader->WaitForLine(m_emlChannelReadableSlot, READ_TIMEOUT_IN_SECONDS);
			}
		}

	} catch(const std::exception& e) {
		ParseEmlFailed(e);
	} catch(...) {
		ParseEmlFailed(boost::unknown_exception());
	}

	return true;
}

MojErr ParseEmlHandler::ParserReady()
{
	if (m_emailParser->HasFatalError()) {
		ParseEmlFailed(MailException("Parsing Error!", __FILE__, __LINE__));
	} else if (m_paused && m_lineReader.get()) {
		m_paused = false;
		m_lineReader->WaitForLine(m_emlChannelReadableSlot, READ_TIMEOUT_IN_SECONDS);
	}

	return MojErrNone;
}

MojErr ParseEmlHandler::ParserDone()
{
	// Update parts for email
	EmailPartList parts;
	m_emailParser->GetFlattenedParts(parts);
	m_email->SetPartList(parts);

	ParseEmlDone(m_emailParser->HasFatalError());

	return MojErrNone;
}

void ParseEmlHandler::Cancel()
{
	CancelImpl();
}

void ParseEmlHandler::CancelImpl()
{
	Complete();
}

void ParseEmlHandler::ParseEmlFailed(const std::exception &e)
{
	MojLogInfo(s_log, "Error Parsing EML: %s", e.what());
	ParseEmlDone(true);
}

void ParseEmlHandler::ParseEmlDone(bool isError)
{
	MojLogInfo(s_log, "parse EML done.  Do cleanup.");

	MojObject reply;
	MojErr err;
	try {
		if(isError) {
			MojLogInfo(s_log, "preparing error response");
			err = reply.putBool(MojServiceMessage::ReturnValueKey, false);
			ErrorToException(err);
			err = reply.putString(MojServiceMessage::ErrorTextKey, "error parsing eml");
			ErrorToException(err);

		} else {
			MojLogInfo(s_log, "preparing result");
			err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
			ErrorToException(err);
			MojObject result;
			EmailAdapter::SerializeToDatabaseObject(*m_email, result);
			err = reply.put("email", result);
			ErrorToException(err);
		}
	} catch(const std::exception& e) {
		reply.putBool(MojServiceMessage::ReturnValueKey, false);
		reply.putString(MojServiceMessage::ErrorTextKey, e.what());
	} catch(...) {
		reply.putBool(MojServiceMessage::ReturnValueKey, false);
		reply.putString(MojServiceMessage::ErrorTextKey, "Unknown Error");
	}

	m_msg->reply(reply);
	m_msg.reset();
	Complete();
}
void ParseEmlHandler::Complete()
{
	MojLogTrace(s_log);

	MojLogInfo(s_log, "completed ParseEmlHandler");

	m_parserReadySlot.cancel();
	m_parserDoneSlot.cancel();
	m_emlChannelReadableSlot.cancel();

	if(m_emlChannel.get()) {
		m_emlChannel->Shutdown();
	}

	if (m_popBusDispatcher != NULL) {
		m_popBusDispatcher->PrepareShutdown();
	}
}


