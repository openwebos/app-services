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

#include "commands/AutoDownloadCommand.h"
#include "client/ImapSession.h"
#include "data/EmailPart.h"
#include "data/DatabaseInterface.h"
#include "data/DatabaseAdapter.h"
#include "data/EmailAdapter.h"
#include "data/EmailSchema.h"
#include "data/ImapEmailAdapter.h"
#include "ImapPrivate.h"
#include "client/DownloadListener.h"
#include "ImapConfig.h"

const size_t AutoDownloadCommand::MAX_AUTODOWNLOAD_SIZE = 300 * 1024; // 300KB

AutoDownloadCommand::AutoDownloadCommand(ImapSession& session, const MojObject& folderId)
: ImapSessionCommand(session),
  m_folderId(folderId),
  m_emailsExamined(0),
  m_getAutoDownloadsSlot(this, &AutoDownloadCommand::GetAutoDownloadsResponse)
{
}

AutoDownloadCommand::~AutoDownloadCommand()
{
}

void AutoDownloadCommand::RunImpl()
{
	GetAutoDownloads();
}


void AutoDownloadCommand::GetAutoDownloads()
{
	m_session.GetDatabaseInterface().GetAutoDownloads(m_getAutoDownloadsSlot, m_folderId, m_page, 100);
}

MojErr AutoDownloadCommand::GetAutoDownloadsResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		BOOST_FOREACH(const MojObject& emailObj, DatabaseAdapter::GetResultsIterators(response)) {
			// If autoDownload is true, consider this email eligible for auto-downloading parts.
			// If it's false then do not attempt to auto-download.
			bool autoDownload = DatabaseAdapter::GetOptionalBool(emailObj, ImapEmailAdapter::AUTO_DOWNLOAD, true);

			if (m_emailsExamined++ >= ImapConfig::GetConfig().GetNumAutoDownloadBodies()) {
				break;
			}

			if(!autoDownload) {
				continue;
			}

			MojObject partsArray;
			err = emailObj.getRequired(EmailSchema::PARTS, partsArray);
			ErrorToException(err);

			EmailPartList emailParts;
			EmailAdapter::ParseParts(partsArray, emailParts);

			BOOST_FOREACH(const EmailPartPtr& part, emailParts) {
				if(part->GetLocalFilePath().empty()) {
					bool fetch = false;

					if(part->IsBodyPart()) {
						fetch = true;
					} else if(part->IsImageType() && part->GetEncodedSize() < MAX_AUTODOWNLOAD_SIZE) {
						fetch = true;
					}

					if(fetch) {
						MojObject emailId;
						err = emailObj.getRequired(DatabaseAdapter::ID, emailId);
						ErrorToException(err);

						MojRefCountedPtr<DownloadListener> listener(NULL);
						m_session.DownloadPart(m_folderId, emailId, part->GetId(), listener, Command::LowPriority);
					}
				}
			}
		}

		if(m_emailsExamined < ImapConfig::GetConfig().GetNumAutoDownloadBodies() && DatabaseAdapter::GetNextPage(response, m_page)) {
			GetAutoDownloads();
		} else {
			Complete();
		}
	} CATCH_AS_FAILURE

	return MojErrNone;
}
