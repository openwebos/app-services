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

#ifndef DOWNLOADLISTENER_H_
#define DOWNLOADLISTENER_H_

#include <string>
#include "core/MojObject.h"
#include "core/MojServiceMessage.h"
#include "CommonErrors.h"

class DownloadListener
{
public:
	DownloadListener(MojServiceMessage* msg, const MojObject& emailId, const MojObject& partId);
	virtual ~DownloadListener();
	void ReportProgress(MojInt64 bytesDownloaded, MojInt64 totalBytes);
	void ReportComplete(MojInt64 bytesDownloaded, MojInt64 totalBytes, const std::string& path, const std::string& mimeType);
	void ReportError(const std::string& errText);
	void ReportError(MojErr err);

protected:
	void BuildResponse(MojObject& response, MojInt64 bytesDownloaded, MojInt64 totalBytes);

	MojRefCountedPtr<MojServiceMessage>	m_msg;

	MojObject	m_emailId;
	MojObject	m_partId;
};

#endif /* DOWNLOADLISTENER_H_ */
