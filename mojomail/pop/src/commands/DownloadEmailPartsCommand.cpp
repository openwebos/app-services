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

#include "commands/DownloadEmailPartsCommand.h"
#include "data/CommonData.h"
#include "data/EmailPart.h"
#include "email/AsyncEmailParser.h"

using namespace std;

const char* const DownloadEmailPartsCommand::COMMAND_STRING			= "RETR";
const int DownloadEmailPartsCommand::DOWNLOAD_PERCENTAGE_TO_REPORT	= 10;

DownloadEmailPartsCommand::DownloadEmailPartsCommand(PopSession& session, int msgNum, int size,
		PopEmail::PopEmailPtr  email,
		boost::shared_ptr<FileCacheClient> fileCacheClient,
		MojRefCountedPtr<Request> request)
: PopMultiLineResponseCommand(session),
  m_fileCacheClient(fileCacheClient),
  m_msgNum(msgNum),
  m_email(email),
  m_msgSize(size),
  m_downloadedSize(0),
  m_request(request),
  m_listener(m_request->GetDownloadListener()),
  m_parseFailed(false),
  m_pause(false),
  m_parserReadySlot(this, &DownloadEmailPartsCommand::ParserReady),
  m_parserDoneSlot(this, &DownloadEmailPartsCommand::ParserDone)
{
	m_includesCRLF = true;
	m_terminationLine.append(CRLF);
	m_handleEndOfResponse = true;

	m_incrementalSize = m_sizeInterval = m_msgSize / DOWNLOAD_PERCENTAGE_TO_REPORT;

	m_emailParser.reset(new AsyncEmailParser());
	m_emailParser->EnableBodyParsing(*fileCacheClient, 4 * 1024);
	m_emailParser->EnablePreviewText(128);
	m_emailParser->SetReadySlot(m_parserReadySlot);
	m_emailParser->SetDoneSlot(m_parserDoneSlot);
}

DownloadEmailPartsCommand::~DownloadEmailPartsCommand()
{
}

void DownloadEmailPartsCommand::RunImpl()
{
	m_emailParser->Begin();

	try{
		// determine part's original mime type first if this download request is for
		// on-demand download
		if (!m_request->GetPartId().null() && !m_request->GetPartId().undefined()) {
			EmailPartList parts =  m_email->GetPartList();
			EmailPartList::iterator itr;
			for (itr = parts.begin(); itr != parts.end(); itr++) {
				EmailPartPtr part = *itr;
				if (part->GetId() == m_request->GetPartId()) {
					m_partMimeType = part->GetMimeType();
					break;
				}
			}
		}

		// Command syntax: "RETR <msg_num>".
		std::ostringstream command;
		command << COMMAND_STRING << " " << m_msgNum;
		SendCommand(command.str());
	}
	catch (const std::exception& e) {
		MojLogError(m_log, "Exception in preparing download email parts request: '%s'", e.what());
		m_parseFailed = true;
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in preparing download email parts request");
		m_parseFailed = true;
		Failure(MailException("Unknown exception in preparing download email parts request", __FILE__, __LINE__));
	}
}

MojErr DownloadEmailPartsCommand::HandleResponse(const std::string& line)
{
	//MojLogInfo(m_log, "*** Body data: '%s'", line.c_str());
	try{
		// update download progress
		UpdateDownloadProgress(line.length());

		if (!isEndOfResponse(line)) {
			if (!line.empty() && line[0] == '.') {
				// Skip dot
				m_emailParser->ParseLine(line.data() + 1, line.length() - 1);
			} else {
				m_emailParser->ParseLine(line.data(), line.length());
			}

			// Wait for filecache, etc
			if (!m_emailParser->IsReadyForData()) {
				PauseResponse();
			}

		} else {
			m_emailParser->End();

			// need to wait for parser to write parts to filecache
			PauseResponse();
		}

	} catch (const std::exception& e) {
		MojLogError(m_log, "Exception in handling download email parts: '%s'", e.what());
		m_parseFailed = true;
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Unknown exception in handling download email parts");
		m_parseFailed = true;
		Failure(MailException("Unknown exception in handling download email parts", __FILE__, __LINE__));
	}
	return MojErrNone;
}


MojErr DownloadEmailPartsCommand::ParserReady()
{
	if(m_emailParser->HasFatalError()) {
		ParseResumeFailed();
		Failure(MailFileCacheNotCreatedException("filecache error when resume parsing!", __FILE__, __LINE__));
	} else {
		//MojLogInfo(m_log, "++++++++ parser is ready");
		ResumeResponse();
	}

	return MojErrNone;
}

MojErr DownloadEmailPartsCommand::ParserDone()
{
	// Save parts list
	EmailPartList parts;
	m_emailParser->GetFlattenedParts(parts);
	m_email->SetPartList(parts);

	string previewText = m_emailParser->GetPreviewText(parts);
	m_email->SetPreviewText(previewText);

	Complete();

	return MojErrNone;
}

void DownloadEmailPartsCommand::ParseFailed()
{
	// mark the email as not in sync window so that sync command won't persist it.
	MojLogInfo(m_log, "Encountered error in parsing email body");
	m_parseFailed = true;
}

void DownloadEmailPartsCommand::ParseResumeFailed()
{
	// mark the email as not in sync window so that sync command won't persist it.
	MojLogInfo(m_log, "Encountered error in filecache creation!");
	m_email->SetDownloaded(true);
	m_parseFailed = true;
}

void DownloadEmailPartsCommand::UpdateDownloadProgress(int dlSize)
{
	if (m_listener.get() && m_downloadedSize >= m_sizeInterval) {
		m_downloadedSize += dlSize;
		m_listener->ReportProgress(m_downloadedSize, m_msgSize);
		m_sizeInterval += m_incrementalSize;
	}
}

void DownloadEmailPartsCommand::CompleteDownloadListener()
{
	try{
		if (m_listener.get()) {
			if (m_parseFailed) {
				MojLogError(m_log, "Replying download body error back to caller");
				m_listener->ReportError("Failed to download email");
			} else {
				if (!m_email->IsDownloaded()) {
					// Get first text part's mime type as the mime type to complete
					// the download request in download listener.
					EmailPartList parts = m_email->GetPartList();
					EmailPartList::iterator itr;
					for (itr = parts.begin(); itr != parts.end(); itr++) {
						EmailPartPtr part = *itr;
						if (part->GetMimeType().find("text") != std::string::npos) {
							m_partMimeType = part->GetMimeType();
							m_partPath = part->GetLocalFilePath();
						}
					}
				}

				MojLogInfo(m_log, "Replying download email completion to caller");
				m_listener->ReportComplete(m_msgSize, m_msgSize, m_partPath, m_partMimeType);
			}
		}
	} catch (const std::exception& e) {
		MojLogError(m_log, "Encountered error in downloading email parts: '%s'", e.what());
		m_parseFailed = true;
		Failure(e);
	} catch (...) {
		MojLogError(m_log, "Encountered error in downloading email parts");
		m_parseFailed = true;
		Failure(MailException("Encountered error in downloading email parts", __FILE__, __LINE__));
	}
}

void DownloadEmailPartsCommand::Cleanup()
{
	CompleteDownloadListener();
}

void DownloadEmailPartsCommand::Status(MojObject& status) const
{
	MojErr err;
	PopMultiLineResponseCommand::Status(status);

	if (m_emailParser.get()) {
		MojObject parserStatus;
		m_emailParser->Status(parserStatus);
		err = status.put("parser", parserStatus);
	}
}
