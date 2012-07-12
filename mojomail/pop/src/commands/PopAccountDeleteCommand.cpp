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

#include "PopDefs.h"
#include "commands/PopAccountDeleteCommand.h"
#include "data/PopFolderAdapter.h"
#include "data/PopEmailAdapter.h"
#include "data/PopAccountAdapter.h"

PopAccountDeleteCommand::PopAccountDeleteCommand(PopClient& client,
												 boost::shared_ptr<DatabaseInterface> dbInterface,
												 const MojObject& payload, MojRefCountedPtr<MojServiceMessage> msg)
: PopClientCommand(client, "Delete POP account", Command::LowPriority),
  m_dbInterface(dbInterface),
  m_payload(payload),
  m_msg(msg),
  m_deletePopAccountSlot(this, &PopAccountDeleteCommand::DeletePopAccountResponse),
  m_smtpAccountDeletedSlot(this, &PopAccountDeleteCommand::SmtpAccountDeletedResponse)
{

}

PopAccountDeleteCommand::~PopAccountDeleteCommand()
{

}

void PopAccountDeleteCommand::RunImpl()
{
	CommandTraceFunction();
	try {
		MojErr err = m_payload.getRequired("accountId", m_accountId);
		ErrorToException(err);
		m_dbInterface->DeleteItems(m_deletePopAccountSlot, PopAccountAdapter::POP_ACCOUNT_KIND, "accountId", m_accountId);
	} catch (const std::exception& e) {
		m_msg->replyError(MojErrInternal);
		Failure(e);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}
}

MojErr PopAccountDeleteCommand::DeletePopAccountResponse(MojObject& response, MojErr err)
{
	CommandTraceFunction();
	try {
		ErrorToException(err);
		m_client.SendRequest(m_smtpAccountDeletedSlot, "com.palm.smtp", "accountDeleted", m_payload);
	} catch (const std::exception& e) {
		m_msg->replyError(err);
		Failure(e);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}

MojErr PopAccountDeleteCommand::SmtpAccountDeletedResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojLogInfo(m_log, "PopAccountDeleteCommand::SmtpAccountDeletedResponse");

		m_client.AccountDeleted();
		m_msg->replySuccess();
		Complete();
	} catch (const std::exception& e) {
		m_msg->replyError(err);
		Failure(e);
	} catch (...) {
		m_msg->replyError(MojErrInternal);
		Failure(MailException("unknown exception", __FILE__, __LINE__));
	}

	return MojErrNone;
}
