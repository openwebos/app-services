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

#ifndef SMTPSIMPLESENDER_H_
#define SMTPSIMPLESENDER_H_

#include "client/SmtpSession.h"
#include "luna/MojLunaService.h"
#include "SmtpValidationListener.h"
#include "CommonErrors.h"

// Note: Why does it needs to be a session?
class SmtpSimpleSender : public SmtpSession, public Command
{
public:
	SmtpSimpleSender(boost::shared_ptr<CommandManager> manager, boost::shared_ptr<SmtpAccount> account, MojService * service, MojServiceMessage* msg, MojString& fromAddress, MojString& toAddress, MojString& payload);
	virtual ~SmtpSimpleSender();

	void LoginSuccess();
	void LoginFailure(const std::string& errorText);
	void SendSuccess();
	void SendFailure(const std::string& errorText);
	void Failure(MailError::ErrorCode errorCode, const std::string& errorText);
	virtual void Failure(const std::string&);
	virtual void Failure(SmtpError error);
	void Disconnected();
	void Run();
	void Cancel();

	MojRefCountedPtr<MojServiceMessage> m_msg;

private:
	MojObject m_protocolSettings;
	MojString m_fromAddress;
	MojString m_toAddress;
	MojString m_payload;
};

#endif /* SMTPSIMPLESENDER_H_ */
