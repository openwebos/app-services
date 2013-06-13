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

#ifndef DOWNLOADLISTENER_H_
#define DOWNLOADLISTENER_H_

#include "core/MojObject.h"
#include "core/MojServiceMessage.h"
#include "core/MojSignal.h"
#include <string>
#include "CommonErrors.h"

class CancelDownloadListener
{
public:
	virtual void CancelDownload() = 0;
};


class DownloadListener : public MojSignalHandler
{
public:
	DownloadListener(MojServiceMessage* msg, const MojObject& emailId, const MojObject& partId);

	void ReportProgress(MojInt64 bytesDownloaded, MojInt64 totalBytes);
	void ReportComplete(MojInt64 bytesDownloaded, MojInt64 totalBytes, const std::string& path, const std::string& mimeType);
	void ReportError(MailError::ErrorCode errorCode, const std::string& errorText);

	bool IsCancelled() const { return m_cancelled; }

	void SetCancelListener(CancelDownloadListener* listener);

protected:
	void BuildResponse(MojObject& response, MojInt64 bytesDownloaded, MojInt64 totalBytes);
	virtual ~DownloadListener();

	MojErr Cancelled(MojServiceMessage* msg);

	MojRefCountedPtr<MojServiceMessage>	m_msg;

	MojObject	m_emailId;
	MojObject	m_partId;
	bool		m_cancelled;
	CancelDownloadListener* m_cancelListener;

	MojSignal<MojServiceMessage*>::Slot<DownloadListener>	m_cancelSlot;
};

#endif /* DOWNLOADLISTENER_H_ */
