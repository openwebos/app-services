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

#ifndef SMTPVALIDATOR_H_
#define SMTPVALIDATOR_H_

#include "client/SmtpSession.h"
#include "luna/MojLunaService.h"
#include "SmtpValidationListener.h"
#include "CommonErrors.h"

class SmtpBusDispatcher;

class SmtpValidator : public SmtpSession, public Command
{
public:
	SmtpValidator(SmtpBusDispatcher* smtpBusDispatcher, boost::shared_ptr<CommandManager> manager, boost::shared_ptr<SmtpAccount> account, MojService * service, MojServiceMessage* msg, MojObject& protocolSettings);
	virtual ~SmtpValidator();
	
	void Run();
	void Cancel();
	void Disconnected();

	void LoginSuccess();
	void LoginFailure(const std::string& errorText);
	
	// Overrides SmtpSession
	void TlsSuccess();

	void Failure(const std::string& errorText);
	virtual void Failure(SmtpSession::SmtpError error);
	void Failure(MailError::ErrorCode errorCode, const std::string& errorText);
	                                        
	void Status(MojObject& status) const;
	static gboolean TimeoutCallback(gpointer data);

	MojRefCountedPtr<MojServiceMessage> m_msg;

private:
	SmtpBusDispatcher*	m_smtpBusDispatcher;
	MojObject			m_protocolSettings;
	bool				m_running;
	uint				m_timeoutId;
	bool				m_replied;
	bool				m_tlsSupported;
};

#endif /* SMTPVALIDATOR_H_ */
