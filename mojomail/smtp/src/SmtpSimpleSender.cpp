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

#include "core/MojServiceMessage.h"
#include "SmtpBusDispatcher.h"
#include "SmtpSimpleSender.h"
#include "SmtpCommon.h"
#include "commands/SimpleSendEmailCommand.h"
#include <sstream>

using namespace std;

// Note: Why does it needs to be a session?
SmtpSimpleSender::SmtpSimpleSender(boost::shared_ptr<CommandManager> manager, boost::shared_ptr<SmtpAccount> account, MojService * service, MojServiceMessage* msg, MojString& fromAddress, MojString& toAddress, MojString& payload)
: SmtpSession(account, service), Command(*manager.get(), Command::NormalPriority), m_msg(msg), m_fromAddress(fromAddress), m_toAddress(toAddress), m_payload(payload)
{
	manager->QueueCommand(this);
}

SmtpSimpleSender::~SmtpSimpleSender()
{

}

void SmtpSimpleSender::Run()
{
	Send();
}

void SmtpSimpleSender::Cancel()
{
}

void SmtpSimpleSender::LoginSuccess()
{
	MojLogInfo(m_log, "Constructing SimpleSendEmailCommand");
	
	std::string fromAddress = m_fromAddress.data();
	std::string toAddress = m_toAddress.data();
	std::string data = m_payload.data();
	
	std::vector<std::string> toAddresses;
	toAddresses.push_back(std::string(toAddress.data()));
                
	MojRefCountedPtr<SimpleSendEmailCommand> command(new SimpleSendEmailCommand(*this, fromAddress, toAddresses, data));
	m_commandManager->QueueCommand(command, true);
}

void SmtpSimpleSender::SendSuccess()
{
	try {
		MojObject reply;
		MojErr err = reply.putBool(MojServiceMessage::ReturnValueKey, true);

		ErrorToException(err);

		err = reply.putBool("errorCode", 0);
		ErrorToException(err);

		err = m_msg->reply(reply);
		ErrorToException(err);
	} catch (const exception& e) {
		m_msg->replyError(MojErrInternal);
	}
	
	RunState(State_SendQuitCommand);
	return;
}

void SmtpSimpleSender::Failure(SmtpError error)
{
	RunState(State_SendQuitCommand);
	Failure(MailError::BAD_USERNAME_OR_PASSWORD, error.errorText);
}

void SmtpSimpleSender::SendFailure(const std::string& errorText)
{
	RunState(State_SendQuitCommand);
	Failure(MailError::INTERNAL_ERROR, errorText);
}

void SmtpSimpleSender::Failure(MailError::ErrorCode errorCode, const std::string& errorText)
{
	MojLogTrace(m_log);

	MojObject reply;
	MojErr err = reply.putBool(MojServiceMessage::ReturnValueKey, false);
	ErrorToException(err);

	err = reply.putInt(MojServiceMessage::ErrorCodeKey, errorCode);
	ErrorToException(err);

	err = reply.putString("errorText", errorText.c_str());
	ErrorToException(err);

	err = m_msg->reply(reply);
	ErrorToException(err);
}

void SmtpSimpleSender::Failure(const std::string& errorText)
{
	Failure(MailError::INTERNAL_ERROR, errorText);
}

void SmtpSimpleSender::Disconnected()
{
	Complete();
}
