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

#include "core/MojServiceMessage.h"
#include "SmtpBusDispatcher.h"
#include "SmtpValidator.h"
#include "SmtpCommon.h"
#include "data/SmtpAccountAdapter.h"
#include <sstream>

using namespace std;

int scount=0;

SmtpValidator::SmtpValidator(SmtpBusDispatcher* smtpBusDispatcher, boost::shared_ptr<CommandManager> manager, boost::shared_ptr<SmtpAccount> account, MojService * service, MojServiceMessage* msg, MojObject& protocolSettings)
: SmtpSession(account, service), 
  Command(*manager.get(), Command::NormalPriority),
  m_msg(msg),
  m_smtpBusDispatcher(smtpBusDispatcher),
  m_protocolSettings(protocolSettings),
  m_running(false),
  m_timeoutId(0),
  m_replied(false),
  m_tlsSupported(false)
{
//	MojLogInfo(m_log, "SmtpValidator %p constructing, %d exist now, will try to connect to %s:%d", this, ++scount, m_account->GetHostname().c_str(), m_account->GetPort());
	manager->QueueCommand(this);
}

SmtpValidator::~SmtpValidator()
{
//	MojLogInfo(m_log, "SmtpValidator %p stopping, was trying to connect to %s:%d", this, m_account->GetHostname().c_str(), m_account->GetPort());
//	MojLogInfo(m_log, "SmtpValidator %p destructing, %d exist now", this, --scount);
}

void SmtpValidator::Run()
{
//	MojLogInfo(m_log, "SmtpValidator %p starting, trying to connect to %s:%d", this, m_account->GetHostname().c_str(), m_account->GetPort());
	Validate();
	m_running = true;
	
	m_timeoutId = g_timeout_add_seconds(15, &SmtpValidator::TimeoutCallback, this);
}

void SmtpValidator::Cancel()
{
}

void SmtpValidator::Status(MojObject& status) const
{
	char buf[64];
	memset(buf, '\0', sizeof(buf));
	snprintf(buf, sizeof(buf)-1, "%p", this);
	MojErr err = status.putString("pointer",buf);;
	ErrorToException(err);

	err = status.putBool("running", m_running);
	ErrorToException(err);
	
	err = status.putString("hostname", m_account->GetHostname().c_str());
	ErrorToException(err);

	err = status.putInt("port", m_account->GetPort());
	ErrorToException(err);

	err = status.putInt("encryption", m_account->GetEncryption());
	ErrorToException(err);
}

gboolean SmtpValidator::TimeoutCallback(gpointer data)
{
	SmtpValidator * that = (SmtpValidator*)data;
	
	that->m_timeoutId = 0; // clear ID before failure case tries to clear it
	
	that->Failure(MailError::INTERNAL_ERROR, "Timeout during validation");
	
	return FALSE;
}

void SmtpValidator::LoginSuccess()
{
	if (m_timeoutId) {
		g_source_remove(m_timeoutId);
		m_timeoutId = 0;
	}

	//MojLogInfo(m_log, "SmtpValidator %p LoginSuccess, was trying to connect to %s:%d", this, m_account->GetHostname().c_str(), m_account->GetPort());
	MojLogInfo(m_log, "SmtpValidator %p LoginSuccess", this);
	try {
		MojObject reply;
		MojErr err;

		err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
		ErrorToException(err);

		if(m_account->GetEncryption() == SmtpAccount::Encrypt_TLSIfAvailable) {
			// Update protocol settings
			MojObject config;
			if(m_protocolSettings.get(SmtpAccountAdapter::CONFIG, config) && !config.null()) {
				err = config.putString(SmtpAccountAdapter::ENCRYPTION,
						m_tlsSupported ? SmtpAccountAdapter::TLS : SmtpAccountAdapter::NO_SSL);
				ErrorToException(err);

				err = m_protocolSettings.put(SmtpAccountAdapter::CONFIG, config);
				ErrorToException(err);
			}
		}

		err = reply.put("protocolSettings", m_protocolSettings);
		ErrorToException(err);

		// create credentials object
		MojObject password;
		err = password.putString("password", m_account->GetPassword().c_str());
		ErrorToException(err);
		MojObject smtp;
		err = smtp.put("smtp", password);
		ErrorToException(err);

		err = reply.put("credentials", smtp);
		ErrorToException(err);

		if (!m_replied) {
			// if we already replied due to a timeout, don't reply again.
			err = m_msg->reply(reply);
			ErrorToException(err);
			
			m_replied = true;
		}
	} catch (const exception& e) {
		if (!m_replied) {
			// if we already replied due to a timeout, don't reply again.
			m_msg->replyError(MojErrInternal);
		
			m_replied = true;
		}
	}
	
	RunState(State_SendQuitCommand);
	return;
}

void SmtpValidator::TlsSuccess()
{
	m_tlsSupported = true;

	SmtpSession::TlsSuccess();
}

void SmtpValidator::Failure(SmtpSession::SmtpError error)
{
	MojLogInfo(m_log, "SmtpValidator %p LoginFailure", this);
	//MojLogInfo(m_log, "SmtpValidator %p LoginFailure, was trying to connect to %s:%d", this, m_account->GetHostname().c_str(), m_account->GetPort());
	
	RunState(State_SendQuitCommand);

	Failure(error.errorCode, error.errorText);
}

void SmtpValidator::Failure(MailError::ErrorCode errorCode, const std::string& errorText)
{
	if (m_timeoutId) {
		g_source_remove(m_timeoutId);
		m_timeoutId = 0;
	}

	MojLogInfo(m_log, "SmtpValidator %p Failure1", this);
	//MojLogInfo(m_log, "SmtpValidator %p Failure1, was trying to connect to %s:%d", this, m_account->GetHostname().c_str(), m_account->GetPort());
	
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
		// if we already replied due to a timeout, don't reply again.
		err = m_msg->reply(reply);
		ErrorToException(err);
		
		m_replied = true;
	}
}

void SmtpValidator::Failure(const std::string& errorText)
{
	MojLogInfo(m_log, "SmtpValidator %p Failure2", this);
	//MojLogInfo(m_log, "SmtpValidator %p Failure2, was trying to connect to %s:%d", this, m_account->GetHostname().c_str(), m_account->GetPort());
	
	Failure(MailError::INTERNAL_ERROR, errorText);
}

void SmtpValidator::Disconnected()
{
	// call PrepareShutdown on bus dispatcher
	if (m_smtpBusDispatcher) {
		m_smtpBusDispatcher->PrepareShutdown();
	}

	Complete();
}
