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

#ifndef FETCHPARTCOMMAND_H_
#define FETCHPARTCOMMAND_H_

#include "commands/ImapSessionCommand.h"
#include "core/MojObject.h"
#include "db/MojDbClient.h"
#include "ImapCoreDefs.h"
#include "protocol/ImapResponseParser.h"
#include "data/CommonData.h"
#include "client/FileCacheClient.h"
#include "stream/BaseOutputStream.h"
#include "util/Timer.h"
#include "client/DownloadListener.h"

class FetchResponseParser;
class AsyncIOChannel;
class FetchPartCommand;
class FileCacheResizerOutputStream;
class PreviewTextExtractorOutputStream;

class ProgressOutputStream : public ChainedOutputStream
{
public:
	ProgressOutputStream(const OutputStreamPtr& sink, FetchPartCommand& command);
	virtual ~ProgressOutputStream();

	// Overrides BaseOutputStream
	void Write(const char* src, size_t length);

protected:
	FetchPartCommand& m_fetchPartCommand;
	size_t	m_totalWritten;
};

class FetchPartCommand : public ImapSessionCommand, public CancelDownloadListener
{
public:
	FetchPartCommand(ImapSession& session, const MojObject& folderId,
			const MojObject& emailId, const MojObject& partId,
			Priority priority = NormalPriority);
	virtual ~FetchPartCommand();

	void RunImpl();
	void AddDownloadListener(const MojRefCountedPtr<DownloadListener>& listener);
	void Progress(size_t totalRead);

	CommandType GetType() const { return CommandType_DownloadPart; }
	bool Equals(const ImapCommand& other) const;

	void Status(MojObject& status) const;

	void Cleanup();

	// Overrides ImapCommand::Cancel
	bool Cancel(CancelType cancel);

	// Implements CancelDownloadListener::CancelDownload
	void CancelDownload();

protected:
	void GetEmail();
	MojErr GetEmailResponse(MojObject& response, MojErr err);

	void SendFetchRequest();
	MojErr FetchResponse();

	void InsertFileCache();
	MojErr InsertFileCacheResponse(MojObject& response, MojErr err);

	void OpenFileChannel();
	MojErr FileChannelClosed();

	void FetchTimeout();

	void FetchOrWriteDone();

	void UpdateEmail();
	MojErr UpdateEmailResponse(MojObject& response, MojErr err);

	virtual void Failure(const std::exception& e);

	void DisableAutoDownload();
	MojErr DisableAutoDownloadResponse(MojObject& response, MojErr err);

	// Whether we should cancel
	bool ShouldCancel();

	void ClearListeners();

	void ReportProgress(MojInt64 totalRead, MojInt64 totalBytes);
	void ReportComplete();
	void Done();

	static const size_t PREVIEW_BUFFER_SIZE;
	static const size_t PREVIEW_TEXT_LENGTH;

	MojObject	m_folderId;
	MojObject	m_emailId;
	MojObject	m_partId;
	std::vector< MojRefCountedPtr<DownloadListener> > m_listeners;

	MojObject	m_partsArray;
	EmailPartPtr	m_part;

	UID			m_uid;
	bool		m_isFirstBodyPart;

	std::string	m_path;

	int			m_totalRead;
	Timer<FetchPartCommand>		m_progressTimer;

	static const int FETCH_PROGRESS_TIMEOUT;

	MojRefCountedPtr<FetchResponseParser>	m_fetchResponseParser;
	MojRefCountedPtr<AsyncIOChannel>		m_fileChannel;
	MojRefCountedPtr<FileCacheResizerOutputStream>		m_fileCacheStream;
	MojRefCountedPtr<PreviewTextExtractorOutputStream>	m_previewTextExtractor;

	bool m_fetchResponseDone;
	bool m_fileChannelClosed;

	bool m_downloadInProgress;

	MojDbClient::Signal::Slot<FetchPartCommand>				m_getEmailSlot;
	FileCacheClient::ReplySignal::Slot<FetchPartCommand>	m_fileCacheSubscriptionSlot;
	ImapResponseParser::DoneSignal::Slot<FetchPartCommand>	m_fetchDoneSlot;
	MojSignal<>::Slot<FetchPartCommand>						m_fileChannelClosedSlot;
	MojDbClient::Signal::Slot<FetchPartCommand>				m_updateEmailSlot;
	MojDbClient::Signal::Slot<FetchPartCommand>				m_disableAutoDownloadSlot;
};

#endif /* FETCHPARTCOMMAND_H_ */
