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

#ifndef SYNCACCOUNTCOMMAND_H_
#define SYNCACCOUNTCOMMAND_H_

#include "commands/PopClientCommand.h"
#include "PopClient.h"

class SyncAccountCommand : public PopClientCommand
{
public:
	SyncAccountCommand(PopClient& client, MojObject& payload);
	virtual ~SyncAccountCommand();

	virtual void RunImpl();
private:
	boost::shared_ptr<PopAccount>					m_account;
	MojObject										m_accountId;
	MojObject										m_idArray;
	MojObject										m_payload;
};

#endif /* SYNCACCOUNTCOMMAND_H_ */
