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

#include "client/DownloadListener.h"
#include "PopDefs.h"

using namespace std;

DownloadListener::DownloadListener(MojServiceMessage* msg, const MojObject& emailId, const MojObject& partId)
: m_msg(msg),
  m_emailId(emailId),
  m_partId(partId)
{
}

DownloadListener::~DownloadListener()
{
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

	err = m_msg->reply(payload);
	ErrorToException(err);
}

void DownloadListener::ReportError(const std::string& errText)
{
	m_msg->replyError(MojErrInternal, errText.c_str());
}

void DownloadListener::ReportError(MojErr err)
{
	m_msg->replyError(err);
}
