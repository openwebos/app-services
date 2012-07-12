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

#include "commands/DownloadPartCommand.h"

DownloadPartCommand::DownloadPartCommand(PopClient& client,
		const MojObject& folderId,
		const MojObject& emailId,
		const MojObject& partId,
		boost::shared_ptr<DownloadListener>& listener)
: PopClientCommand(client, "Download POP email"),
  m_folderId(folderId),
  m_emailId(emailId),
  m_partId(partId),
  m_listener(listener)
{

}

DownloadPartCommand::~DownloadPartCommand()
{

}

void DownloadPartCommand::RunImpl()
{
	try {
		if (!m_client.GetAccount().get()) {
			MojString err;
			err.format("Account is not loaded for '%s'", AsJsonString(m_client.GetAccountId()).c_str());
			throw MailException(err.data(), __FILE__, __LINE__);
		}

		m_client.GetSession()->FetchEmail(m_emailId, m_partId, m_listener);
		Complete();
	} catch (const std::exception& ex) {
		Failure(ex);
	} catch (...) {
		Failure(MailException("Unknown exception in downloading email", __FILE__, __LINE__));
	}
}
