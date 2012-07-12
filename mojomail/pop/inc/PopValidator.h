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

#ifndef POPVALIDATOR_H_
#define POPVALIDATOR_H_

#include "client/PopSession.h"
#include "luna/MojLunaService.h"
#include "PopValidationListener.h"
#include "CommonErrors.h"

class PopBusDispatcher;
class PopConfig;
class PopSessionCommand;
class ConnectionFactory;
class CommandFactory;
class DatabaseInterface;

class PopValidator : public PopSession, public Command
{
public:
	PopValidator(PopBusDispatcher* busDispatcher,
			MojRefCountedPtr<CommandManager> manager,
			boost::shared_ptr<PopAccount> account,
			MojServiceMessage* msg,
			MojObject& protocolSettings);
	virtual ~PopValidator();
	
	void Run();
	void Cancel();
	void Disconnected();

	void LoginSuccess();
	void LoginFailure(MailError::ErrorCode errCode, const std::string& errorText);
	virtual void ConnectFailure(MailError::ErrorCode errCode, const std::string& errMsg);
	void Failure(MailError::ErrorCode errorCode, const std::string& errorText);
	void Failure(const std::string& errorText);
	void Status(MojObject& status) const;
	static gboolean TimeoutCallback(gpointer data);

private:
	MojRefCountedPtr<MojServiceMessage> m_msg;

	MojObject		m_protocolSettings;
	bool			m_running;
	unsigned int	m_timeoutId;
	bool			m_replied;
	PopBusDispatcher*		m_popBusDispatcher;
};

#endif /* POPVALIDATOR_H_ */
