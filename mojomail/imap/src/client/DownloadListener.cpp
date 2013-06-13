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

#include "client/DownloadListener.h"
#include "ImapCoreDefs.h"

using namespace std;

DownloadListener::DownloadListener(MojServiceMessage* msg, const MojObject& emailId, const MojObject& partId)
: m_msg(msg),
  m_emailId(emailId),
  m_partId(partId),
  m_cancelled(false),
  m_cancelListener(NULL),
  m_cancelSlot(this, &DownloadListener::Cancelled)
{
	if(m_msg.get() != NULL) {
		m_msg->notifyCancel(m_cancelSlot);
	}
}

DownloadListener::~DownloadListener()
{
}

void DownloadListener::SetCancelListener(CancelDownloadListener* cancelListener)
{
	m_cancelListener = cancelListener;
}

// This is called when the bus subscription is called (e.g. cancelled from UI)
MojErr DownloadListener::Cancelled(MojServiceMessage* msg)
{
	m_cancelled = true;

	if(m_cancelListener)
		m_cancelListener->CancelDownload();

	return MojErrNone;
}

void DownloadListener::BuildResponse(MojObject& payload, MojInt64 bytesDownloaded, MojInt64 totalBytes)
{
	MojErr err;

	err = payload.put("emailId", m_emailId);
	ErrorToException(err);

	if(!m_partId.undefined()) {
		err = payload.put("partId", m_partId);
		ErrorToException(err);
	}

	err = payload.put("bytesDownloaded", bytesDownloaded);
	ErrorToException(err);

	err = payload.put("totalBytes", totalBytes);
	ErrorToException(err);
}

void DownloadListener::ReportProgress(MojInt64 bytesDownloaded, MojInt64 totalBytes)
{
	MojErr err;
	MojObject payload;

	BuildResponse(payload, bytesDownloaded, totalBytes);

	err = m_msg->reply(payload);
	ErrorToException(err);
}

void DownloadListener::ReportComplete(MojInt64 bytesDownloaded, MojInt64 totalBytes, const string& path, const string& mimeType)
{
	MojErr err;
	MojObject payload;

	BuildResponse(payload, bytesDownloaded, totalBytes);

	err = payload.putString("path", path.c_str());
	ErrorToException(err);

	err = payload.putString("mimeType", mimeType.c_str());
	ErrorToException(err);

	err = m_msg->replySuccess(payload);
	ErrorToException(err);
}

void DownloadListener::ReportError(MailError::ErrorCode errorCode, const string& errorText)
{
	MojErr err;
	MojObject payload;

	err = payload.put("returnValue", false);
	ErrorToException(err);

	err = payload.put("emailId", m_emailId);
	ErrorToException(err);

	if(!m_partId.undefined()) {
		err = payload.put("partId", m_partId);
		ErrorToException(err);
	}

	err = payload.put("errorCode", errorCode);
	ErrorToException(err);

	err = payload.putString("errorText", errorText.c_str());
	ErrorToException(err);

	err = m_msg->reply(payload);
	ErrorToException(err);
}
