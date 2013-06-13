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

#include "core/MojServiceMessage.h"
#include "PopBusDispatcher.h"
#include "PopValidator.h"
#include "PopDefs.h"
#include <sstream>

using namespace std;

PopValidator::PopValidator(PopBusDispatcher* busDispatcher,
		MojRefCountedPtr<CommandManager> manager,
		boost::shared_ptr<PopAccount> account,
		MojServiceMessage* msg,
		MojObject& protocolSettings)
: PopSession(account), Command(*manager.get(), Command::NormalPriority), m_msg(msg), m_protocolSettings(protocolSettings),
  m_running(false),
  m_timeoutId(0),
  m_replied(false),
  m_popBusDispatcher(busDispatcher)
{
	manager->QueueCommand(this);
}

PopValidator::~PopValidator()
{
	if(m_timeoutId > 0) {
		g_source_remove(m_timeoutId);
		m_timeoutId = 0;
	}
}

void PopValidator::Run()
{
	Validate();
	m_running = true;
	
	m_timeoutId = g_timeout_add_seconds(15, &PopValidator::TimeoutCallback, this);
}

void PopValidator::Cancel()
{
}

void PopValidator::Status(MojObject& status) const
{
	std::ostringstream p;
	p << this;
	MojErr err = status.putString("pointer", p.str().c_str());;
	ErrorToException(err);

	err = status.putBool("running", m_running);
	ErrorToException(err);

	err = status.putString("hostname", m_account->GetHostName().c_str());
	ErrorToException(err);

	err = status.putInt("port", m_account->GetPort());
	ErrorToException(err);

	err = status.putString("encryption", m_account->GetEncryption().c_str());
	ErrorToException(err);
}

gboolean PopValidator::TimeoutCallback(gpointer data)
{
	PopValidator * that = (PopValidator*)data;

	that->m_timeoutId = 0; // clear ID before failure case tries to clear it

	that->Failure(MailError::INTERNAL_ERROR, "Timeout during validation");

	return FALSE;
}

void PopValidator::ConnectFailure(MailError::ErrorCode errCode, const std::string& errMsg)
{
	m_state = State_NeedsConnection;
	Failure(errCode, errMsg);
}

void PopValidator::LoginSuccess()
{
	if (m_timeoutId) {
		g_source_remove(m_timeoutId);
		m_timeoutId = 0;
	}

	try {
		MojObject reply;
		MojErr err = reply.putBool(MojServiceMessage::ReturnValueKey, true);

		ErrorToException(err);

		err = reply.putInt("errorCode", 0);
		ErrorToException(err);

		err = reply.put("protocolSettings", m_protocolSettings);
		ErrorToException(err);

		// create credentials object
		MojObject password;
		err = password.putString("password", m_account->GetPassword().c_str());
		ErrorToException(err);
		MojObject common;
		err = common.put("common", password);
		ErrorToException(err);

		err = reply.put("credentials", common);
		ErrorToException(err);

		if (!m_replied) {
			err = m_msg->reply(reply);
			ErrorToException(err);
			m_replied = true;
		}

		m_state = State_LogOut;
		CheckQueue();
	} catch (const exception& e) {
		if (!m_replied) {
			m_msg->replyError(MojErrInternal);
			m_replied = true;
		}
	}

	return;
}

void PopValidator::LoginFailure(MailError::ErrorCode errorCode, const std::string& errorText)
{
	m_state = State_InvalidCredentials;
	Failure(MailError::BAD_USERNAME_OR_PASSWORD, errorText);
}

void PopValidator::Failure(MailError::ErrorCode errorCode, const std::string& errorText)
{
	if (m_timeoutId) {
		g_source_remove(m_timeoutId);
		m_timeoutId = 0;
	}

	MojLogTrace(m_log);

	MojObject reply;
	MojErr err = reply.putBool(MojServiceMessage::ReturnValueKey, false);
	ErrorToException(err);

	std::string accountErrorCode = MailError::GetAccountErrorCode(errorCode);

	err = reply.putString(MojServiceMessage::ErrorCodeKey, accountErrorCode.c_str());
	ErrorToException(err);

	err = reply.putInt("mailErrorCode", errorCode);
	ErrorToException(err);

	err = reply.putString("errorText", errorText.c_str());
	ErrorToException(err);

	if (!m_replied) {
		err = m_msg->reply(reply);
		ErrorToException(err);
		m_replied = true;
	}

	if (m_state > State_Connecting) {
		m_state = State_LogOut;
		CheckQueue();
	}
}

void PopValidator::Failure(const std::string& errorText)
{
	Failure(MailError::INTERNAL_ERROR, errorText);
}

void PopValidator::Disconnected()
{
	Complete();

	// inform POP client that the current session has been shut down.
	m_canShutdown = true;
	if (m_popBusDispatcher != NULL) {
		m_popBusDispatcher->PrepareShutdown();
	}
}
