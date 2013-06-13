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

#include "activity/ActivityBuilderFactory.h"
#include "PopClient.h"
#include "commands/PopClientCommand.h"
#include "PopDefs.h"

class PopAccountUpdateCommand : public PopClientCommand
{
public:
	PopAccountUpdateCommand(PopClient& client, MojObject& payload, bool credentialsChanged);
	virtual ~PopAccountUpdateCommand();
	virtual void RunImpl();

private:
	void 	UpdateAccountWatchActivity();
	void 	Sync();
	void	GetPassword();

	MojErr 	GetAccountResponse(MojObject& response, MojErr err);
	MojErr 	CreateActivityResponse(MojObject& response, MojErr err);
	MojErr	GetPasswordResponse(MojObject& response, MojErr err);

	void NotifySmtp();
	MojErr NotifySmtpResponse(MojObject& response, MojErr err);

	void Done();

	MojObject														m_payload;
	bool															m_credentialsChanged;

	MojDbClient::Signal::Slot<PopAccountUpdateCommand>				m_getAccountSlot;
	MojServiceRequest::ReplySignal::Slot<PopAccountUpdateCommand> 	m_getPasswordSlot;
	MojDbClient::Signal::Slot<PopAccountUpdateCommand>				m_createActivitySlot;
	MojServiceRequest::ReplySignal::Slot<PopAccountUpdateCommand>	m_notifySmtpSlot;

	bool															m_callSyncFolderOnClient;
	MojRefCountedPtr<ActivityBuilderFactory>						m_activityBuilderFactory;

	MojObject														m_accountId;
	MojObject														m_transportObj;
	MojObject														m_accountRev;
	MojRefCountedPtr<Activity>										m_accountWatchActivity;

};
