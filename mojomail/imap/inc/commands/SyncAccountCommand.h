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

#ifndef SYNCACCOUNTCOMMAND_H_
#define SYNCACCOUNTCOMMAND_H_

#include "commands/ImapClientCommand.h"
#include "activity/Activity.h"
#include "client/SyncParams.h"

class SyncAccountCommand : public ImapClientCommand
{
public:
	SyncAccountCommand(ImapClient& client, SyncParams syncParams);
	virtual ~SyncAccountCommand();

protected:
	void RunImpl();

	SyncParams		m_syncParams;
	ActivityPtr		m_activity;
};

#endif /* SYNCACCOUNTCOMMAND_H_ */
