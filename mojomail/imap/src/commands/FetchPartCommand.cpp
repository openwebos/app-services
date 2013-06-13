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

#include "commands/FetchPartCommand.h"
#include "ImapPrivate.h"
#include "client/ImapSession.h"
#include "data/DatabaseAdapter.h"
#include "data/DatabaseInterface.h"
#include "data/EmailAdapter.h"
#include "data/EmailPart.h"
#include "data/EmailSchema.h"
#include "data/ImapEmailAdapter.h"
#include <sstream>
#include "protocol/FetchResponseParser.h"
#include "async/GIOChannelWrapper.h"
#include "async/FileCacheResizerOutputStream.h"
#include "client/DownloadListener.h"
#include <boost/algorithm/string/trim.hpp>
#include "stream/PreviewTextExtractorOutputStream.h"
#include "stream/Base64DecoderOutputStream.h"
#include "stream/QuotePrintableDecoderOutputStream.h"
#include "stream/UTF8DecoderOutputStream.h"
#include "email/PreviewTextGenerator.h"
#include "exceptions/ExceptionUtils.h"
#include "util/StringUtils.h"

const size_t FetchPartCommand::PREVIEW_BUFFER_SIZE = 8192;
const size_t FetchPartCommand::PREVIEW_TEXT_LENGTH = 128;

const int FetchPartCommand::FETCH_PROGRESS_TIMEOUT = 120; // 2 minutes with no updates

FetchPartCommand::FetchPartCommand(ImapSession& session,
	const MojObject& folderId, const MojObject& emailId, const MojObject& partId, Priority priority)
: ImapSessionCommand(session, priority),
  m_emailId(emailId),
  m_partId(partId),
  m_uid(0),
  m_isFirstBodyPart(false),
  m_totalRead(0),
  m_fetchResponseDone(false),
  m_fileChannelClosed(false),
  m_downloadInProgress(false),
  m_getEmailSlot(this, &FetchPartCommand::GetEmailResponse),
  m_fileCacheSubscriptionSlot(this, &FetchPartCommand::InsertFileCacheResponse),
  m_fetchDoneSlot(this, &FetchPartCommand::FetchResponse),
  m_fileChannelClosedSlot(this, &FetchPartCommand::FileChannelClosed),
  m_updateEmailSlot(this, &FetchPartCommand::UpdateEmailResponse),
  m_disableAutoDownloadSlot(this, &FetchPartCommand::DisableAutoDownloadResponse)
{
}

FetchPartCommand::~FetchPartCommand()
{
	ClearListeners();
}

void FetchPartCommand::AddDownloadListener(const MojRefCountedPtr<DownloadListener>& listener)
{
	m_listeners.push_back(listener);
	listener->SetCancelListener(this);
}

void FetchPartCommand::ClearListeners()
{
	BOOST_FOREACH(const MojRefCountedPtr<DownloadListener>& listener, m_listeners) {
		listener->SetCancelListener(NULL);
	}
	
	m_listeners.clear();
}

bool FetchPartCommand::Equals(const ImapCommand& other) const
{
	if(other.GetType() != CommandType_DownloadPart) {
		return false;
	}

	const FetchPartCommand& otherFetch = static_cast<const FetchPartCommand&>(other);
	if(m_emailId == otherFetch.m_emailId && m_partId == otherFetch.m_partId) {
		return true;
	}

	return false;
}

void FetchPartCommand::RunImpl()
{
	CommandTraceFunction();

	GetEmail();
}

void FetchPartCommand::GetEmail()
{
	CommandTraceFunction();

	m_session.GetDatabaseInterface().GetById(m_getEmailSlot, m_emailId);
}

MojErr FetchPartCommand::GetEmailResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		MojObject emailObj;
		DatabaseAdapter::GetOneResult(response, emailObj);

		err = emailObj.getRequired(ImapEmailAdapter::UID, m_uid);
		ErrorToException(err);

		MojObject partsObj;
		err = emailObj.getRequired(EmailSchema::PARTS, partsObj);
		ErrorToException(err);

		// Save a copy of the parts list
		m_partsArray = partsObj;

		EmailPartList partList;
		EmailAdapter::ParseParts(partsObj, partList);

		bool wantBodyPart = m_partId.undefined() || m_partId.null();

		m_isFirstBodyPart = true;

		// Find part, or body if no part specified
		BOOST_FOREACH(const EmailPartPtr& part, partList) {
			if((wantBodyPart && part->IsBodyPart())
			|| (part->GetId() == m_partId)) {
				m_part = part;
				m_partId = part->GetId();
				break;
			}

			if(part->IsBodyPart()) {
				m_isFirstBodyPart = false;
			}
		}

		if(!m_part.get()) {
			// not found!
			stringstream ss;
			if(wantBodyPart) {
				ss << "requested body part";
			} else {
				ss << "requested part id " << AsJsonString(m_partId);
			}
			ss << " for email " << AsJsonString(m_emailId) << " not found";
			throw MailException(ss.str().c_str(), __FILE__, __LINE__);
		}

		if(m_part->GetSection().empty()) {
			// missing section
			throw MailException("requested part missing section field", __FILE__, __LINE__);
		}

		if(ShouldCancel()) {
			throw MailException("download cancelled by user", __FILE__, __LINE__);
		}

		InsertFileCache();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void FetchPartCommand::InsertFileCache()
{
	CommandTraceFunction();

	MojInt64 lifetime = -1; // no lifetime
	MojInt64 cost = -1; // default cost
	MojInt64 size = m_part->EstimateMaxSize(); // needs to be >= actual size

	string filename = m_part->GetDisplayName();

	if(filename.empty()) {
		string prefix = m_part->IsBodyPart() ? "body" : "attachment";

		filename = prefix + m_part->GuessFileExtension();
	} else {
		StringUtils::SanitizeFileCacheName(filename);
	}

	const char* type = m_part->IsBodyPart() ? FileCacheClient::EMAIL : FileCacheClient::ATTACHMENT;

	m_session.GetFileCacheClient().InsertCacheObject(m_fileCacheSubscriptionSlot, type, filename.c_str(), size, cost, lifetime);
}

MojErr FetchPartCommand::InsertFileCacheResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		MojString pathName;
		err = response.getRequired("pathName", pathName);
		ErrorToException(err);

		m_path.assign(pathName.data());

		OpenFileChannel();
		SendFetchRequest();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void FetchPartCommand::SendFetchRequest()
{
	CommandTraceFunction();

	stringstream ss;
	ss << "UID FETCH " << m_uid << " (BODY.PEEK[" << m_part->GetSection() << "])";

	m_fetchResponseParser.reset(new FetchResponseParser(m_session, m_fetchDoneSlot));

	OutputStreamPtr os = m_fileChannel->GetOutputStream();

	// Set up decoders, in reverse order so that the first wrapped stream gets called
	// last (after everything else has been decoded)

	m_fileCacheStream.reset(new FileCacheResizerOutputStream(os, m_session.GetFileCacheClient(), m_path, m_part->EstimateMaxSize()));
	os = m_fileCacheStream;

	// Extract preview text from decoded output
	// Only do this for the first body part
	if(m_part->IsBodyPart() && m_isFirstBodyPart) {
		m_previewTextExtractor.reset(new PreviewTextExtractorOutputStream(os, PREVIEW_BUFFER_SIZE));
		os = m_previewTextExtractor;
	}

	if(m_part->IsBodyPart()) {
		// Convert charset to UTF-8
		string charset = m_part->GetCharset();

		if(charset.empty()) {
			charset = "us-ascii";
		}

		try {
			os.reset(new UTF8DecoderOutputStream(os, charset.c_str()));
		} catch (const std::exception& e) {
			MojLogError(m_log, "error creating charset decoder stream: %s", e.what());

			// Fall back to ASCII
			os.reset(new UTF8DecoderOutputStream(os, "us-ascii"));
		}
	}

	// Decode content-encoding
	const string& encoding = m_part->GetEncoding();

	if(encoding == "base64") {
		os.reset(new Base64DecoderOutputStream(os));
	} else if(encoding == "quoted-printable") {
		os.reset(new QuotePrintableDecoderOutputStream(os));
	}

	os.reset(new ProgressOutputStream(os, *this));

	m_fetchResponseParser->SetPartOutputStream(os);

	m_downloadInProgress = true;

	m_progressTimer.SetTimeout(FETCH_PROGRESS_TIMEOUT, this, &FetchPartCommand::FetchTimeout);

	m_session.SendRequest(ss.str(), m_fetchResponseParser);
}

MojErr FetchPartCommand::FetchResponse()
{
	CommandTraceFunction();
	m_fetchResponseDone = true;

	try {
		ImapStatusCode status = m_fetchResponseParser->GetStatus();

		if(status == OK) {
			FetchOrWriteDone();
		} else if(status == BAD || status == NO) {
			// FIXME report error; might be only fatal to this request (email deleted)
			m_fetchResponseParser->CheckStatus();
		} else {
			m_fetchResponseParser->CheckStatus();
		}

	} CATCH_AS_FAILURE

	return MojErrNone;
}

void FetchPartCommand::OpenFileChannel()
{
	CommandTraceFunction();

	assert( !m_path.empty() );

	MojLogInfo(m_log, "writing email %s %s to %s",
			AsJsonString(m_emailId).c_str(), m_part->IsBodyPart() ? "body" : "attachment", m_path.c_str());

	// Open file
	GIOChannelWrapperFactory factory;
	m_fileChannel = factory.OpenFile(m_path.c_str(), "w");
	m_fileChannel->WatchClosed(m_fileChannelClosedSlot);
}

void FetchPartCommand::FetchTimeout()
{
	MojLogError(m_log, "fetch timed out after %d seconds with no updates", FETCH_PROGRESS_TIMEOUT);

	if(m_part.get()) {
		MojLogInfo(m_log, "fetch done = %d, file closed = %d, %d/%lld bytes handled",
				m_fetchResponseDone, m_fileChannelClosed, m_totalRead, m_part->GetEncodedSize());
	}

	m_fetchDoneSlot.cancel();
	m_fileChannelClosedSlot.cancel();

	MailException exc("fetch timed out", __FILE__, __LINE__);

	Failure(exc);
}

MojErr FetchPartCommand::FileChannelClosed()
{
	CommandTraceFunction();
	m_fileChannelClosed = true;

	try {
		// Cancel slot to indicate that we're done
		m_fileCacheSubscriptionSlot.cancel();

		FetchOrWriteDone();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void FetchPartCommand::FetchOrWriteDone()
{
	CommandTraceFunction();

	// The response finish and file channel closures are asynchronous
	// Need to make sure both are done (possibly failed?) before continuing.
	if(m_fetchResponseDone && m_fileChannelClosed) {
		MojLogDebug(m_log, "done with fetch and write, updating email");

		m_downloadInProgress = false;
		m_progressTimer.Cancel();

		UpdateEmail();
	} else {
		MojLogDebug(m_log, "partially done, fetch=%d, channel=%d",
				m_fetchResponseDone, m_fileChannelClosed);
	}
}

void FetchPartCommand::UpdateEmail()
{
	CommandTraceFunction();

	MojErr err;
	MojObject emailObj;

	MojObject::ArrayIterator it;
	err = m_partsArray.arrayBegin(it);
	ErrorToException(err);

	err = emailObj.put(DatabaseAdapter::ID, m_emailId);
	ErrorToException(err);

	for (; it != m_partsArray.arrayEnd(); ++it) {
		MojObject& partObj = *it;

		MojObject partId;
		err = partObj.getRequired(DatabaseAdapter::ID, partId);
		ErrorToException(err);

		if(partId == m_part->GetId()) {
			// Update path
			err = partObj.putString(EmailSchema::Part::PATH, m_path.c_str());
			ErrorToException(err);
		}
	}

	err = emailObj.put(EmailSchema::PARTS, m_partsArray);
	ErrorToException(err);

	// Update preview text
	if(m_previewTextExtractor.get()) {
		const string& previewBuf = m_previewTextExtractor->GetPreviewText();

		bool isHtml = boost::iequals(m_part->GetMimeType(), "text/html");

		string previewText = PreviewTextGenerator::GeneratePreviewText(previewBuf, PREVIEW_TEXT_LENGTH, isHtml);
		emailObj.putString(EmailSchema::SUMMARY, previewText.c_str());
	}

	m_session.GetDatabaseInterface().UpdateEmail(m_updateEmailSlot, emailObj);
}

MojErr FetchPartCommand::UpdateEmailResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();

	try {
		ResponseToException(response, err);

		// TODO: handle "ninja" writes

		MojLogInfo(m_log, "done writing email %s part", AsJsonString(m_emailId).c_str());

		Done();
	} CATCH_AS_FAILURE

	return MojErrNone;
}

void FetchPartCommand::Done()
{
	ReportComplete();

	Complete();
}

void FetchPartCommand::Failure(const exception& e)
{
	try {
		// Make sure these don't fire again after failing
		m_fetchDoneSlot.cancel();
		m_fileChannelClosedSlot.cancel();
		m_fileCacheSubscriptionSlot.cancel();

		MojLogError(m_log, "error downloading part: %s", e.what());

		MailError::ErrorInfo errorInfo = ExceptionUtils::GetErrorInfo(e);

		// Report error to UI
		BOOST_FOREACH(const MojRefCountedPtr<DownloadListener>& listener, m_listeners) {
			listener->ReportError(errorInfo.errorCode, errorInfo.errorText);
		}

		if(m_downloadInProgress) {
			m_downloadInProgress = false;

			// Fatal because the parser may not be able to recover
			m_session.FatalError("failure while downloading part");
		}

		if(m_uid > 0 && !IsConnectionError(errorInfo.errorCode)) {
			// If we have a valid email object, and the error wasn't a connection failure, set autoDownload = false

			// TODO: Expire file cache object
			DisableAutoDownload();
		} else {
			ImapSessionCommand::Failure(e);
		}

	} catch(const std::exception& e) {
		// This shouldn't happen
		MojLogError(m_log, "exception in %s:%d: %s", __FILE__, __LINE__, e.what());

		Complete();
	} catch(...) {
		// This *really* shouldn't happen
		MojLogError(m_log, "unknown exception in %s:%d", __FILE__, __LINE__);
		Complete();
	}
}

void FetchPartCommand::DisableAutoDownload()
{
	MojErr err;

	MojObject emailObj;

	err = emailObj.put(DatabaseAdapter::ID, m_emailId);
	ErrorToException(err);

	err = emailObj.put(ImapEmailAdapter::AUTO_DOWNLOAD, false);
	ErrorToException(err);

	m_session.GetDatabaseInterface().UpdateEmail(m_disableAutoDownloadSlot, emailObj);
}

MojErr FetchPartCommand::DisableAutoDownloadResponse(MojObject& response, MojErr err)
{
	try {
		ResponseToException(response, err);

		Complete();
	} catch(const std::exception& e) {
		// Must use the base class Failure() here or we'll go into a loop
		ImapSessionCommand::Failure(e);
	}

	return MojErrNone;
}

bool FetchPartCommand::Cancel(CancelType cancelType)
{
	MailError::ErrorInfo errorInfo = GetCancelErrorInfo(cancelType);

	if (IsRunning()) {
		MailNetworkDisconnectionException exc("download cancelled", __FILE__, __LINE__);
		Failure(exc);
	} else {
		// Report error to UI
		BOOST_FOREACH(const MojRefCountedPtr<DownloadListener>& listener, m_listeners) {
			listener->ReportError(errorInfo.errorCode, errorInfo.errorText);
		}

		Complete();
	}

	return true;
}

bool FetchPartCommand::ShouldCancel()
{
	unsigned int cancelVotes = 0;

	BOOST_FOREACH(const MojRefCountedPtr<DownloadListener>& listener, m_listeners) {
		if(listener->IsCancelled()) {
			cancelVotes++;
		}
	}

	// Note that if there's no download listeners, it's OK
	if(cancelVotes > 0 && cancelVotes == m_listeners.size()) {
		return true;
	} else {
		return false;
	}
}

void FetchPartCommand::CancelDownload()
{
	CommandTraceFunction();

	if(m_downloadInProgress && ShouldCancel()) {
		MojLogInfo(m_log, "cancelling part download; all listeners unsubscribed");

		m_downloadInProgress = false;

		// Kill the connection to abort the download
		m_session.ForceReconnect("cancelling attachment download");

		// Fail the command *after* calling disconnect, so it doesn't try to run new commands
		MailNetworkDisconnectionException exc("download cancelled by user", __FILE__, __LINE__);
		Failure(exc);
	}
}

void FetchPartCommand::Progress(size_t totalRead)
{
	MojInt64 totalBytes = m_part->GetEncodedSize();

	m_totalRead = totalRead;

	m_progressTimer.SetTimeout(FETCH_PROGRESS_TIMEOUT, this, &FetchPartCommand::FetchTimeout);

	if(totalRead == totalBytes) {
		// Don't need any more data from the server
		m_downloadInProgress = false;
	}

	if(totalRead < totalBytes) {
		ReportProgress(totalRead, totalBytes);
	}
}

void FetchPartCommand::ReportProgress(MojInt64 totalRead, MojInt64 totalBytes)
{
	BOOST_FOREACH(const MojRefCountedPtr<DownloadListener>& listener, m_listeners) {
		if(listener->IsCancelled()) continue;

		listener->ReportProgress(totalRead, totalBytes);
	}
}

void FetchPartCommand::ReportComplete()
{
	BOOST_FOREACH(const MojRefCountedPtr<DownloadListener>& listener, m_listeners) {
		if(listener->IsCancelled()) continue;

		listener->ReportComplete(m_totalRead, m_totalRead, m_path, m_part->GetMimeType());
	}
}

ProgressOutputStream::ProgressOutputStream(const OutputStreamPtr& sink, FetchPartCommand& command)
: ChainedOutputStream(sink),
  m_fetchPartCommand(command),
  m_totalWritten(0)
{
}

ProgressOutputStream::~ProgressOutputStream()
{
}

void ProgressOutputStream::Write(const char* src, size_t length)
{
	ChainedOutputStream::Write(src, length);

	m_totalWritten += length;
	m_fetchPartCommand.Progress(m_totalWritten);
}

void FetchPartCommand::Status(MojObject& status) const
{
	MojErr err;
	ImapSessionCommand::Status(status);

	err = status.put("emailId", m_emailId);
	ErrorToException(err);

	err = status.put("partId", m_partId);
	ErrorToException(err);

	MojObject partInfo;

	if(m_uid > 0) {
		err = partInfo.put("uid", (MojInt64) m_uid);
		ErrorToException(err);
	}

	if(!m_path.empty()) {
		err = partInfo.putString("path", m_path.c_str());
		ErrorToException(err);
	}

	if(m_part.get()) {
		err = partInfo.putString("type", m_part->IsBodyPart() ? "body" : "attachment");
		ErrorToException(err);

		err = partInfo.put("bytesTotal", m_part->GetEncodedSize());
		ErrorToException(err);

		err = partInfo.put("bytesRead", m_totalRead);
		ErrorToException(err);
	}

	err = status.put("partInfo", partInfo);
	ErrorToException(err);

	if(m_fileChannel.get()) {
		MojObject fileChannelStatus;

		// FIXME add status to AsyncIOChannel
		GIOChannelWrapper* channel = dynamic_cast<GIOChannelWrapper*>(m_fileChannel.get());

		if(channel) {
			channel->Status(fileChannelStatus);
			err = status.put("fileChannel", fileChannelStatus);
			ErrorToException(err);
		}
	}
}

void FetchPartCommand::Cleanup()
{
	m_fileCacheSubscriptionSlot.cancel();
	m_fileChannelClosedSlot.cancel();
	m_fileChannel.reset();

	m_progressTimer.Cancel();

	ClearListeners();
}
