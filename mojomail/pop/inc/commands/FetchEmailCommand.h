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

#ifndef FETCHEMAILCOMMAND_H_
#define FETCHEMAILCOMMAND_H_

#include "commands/DownloadEmailPartsCommand.h"
#include "commands/PopSessionPowerCommand.h"
#include "core/MojObject.h"
#include "db/MojDbClient.h"
#include "data/PopEmail.h"
#include "data/PopEmailAdapter.h"
#include "request/Request.h"

class FetchEmailCommand : public PopSessionPowerCommand
{
public:
	FetchEmailCommand(PopSession& session, Request::RequestPtr request);
	~FetchEmailCommand();

	virtual void 	RunImpl();
	MojErr			LoadEmailResponse(MojObject& response, MojErr err);
	MojErr			ClearPreviousPartsResponse(MojObject& response, MojErr err);
	MojErr 			GetEmailBodyResponse();
	MojErr			UpdateEmailSummaryResponse(MojObject& response, MojErr err);
	MojErr			UpdateEmailPartsResponse(MojObject& response, MojErr err);

	void Status(MojObject& status) const;
private:
	void			LoadEmail();
	void			ClearPreviousParts();
	void			FetchEmailBody();
	void			UpdateEmailSummary(const PopEmail::PopEmailPtr& email);
	void			UpdateEmailParts(const PopEmail::PopEmailPtr& email);
	virtual void	Complete();
	virtual	void	Failure(const std::exception& ex);

	Request::RequestPtr								m_request;
	PopEmail::PopEmailPtr							m_email;

	MojRefCountedPtr<DownloadEmailPartsCommand> 	m_downloadBodyCommand;
	MojRefCountedPtr<PopCommandResult>				m_downloadBodyResult;

	MojDbClient::Signal::Slot<FetchEmailCommand> 	m_loadEmailResponseSlot;
	MojDbClient::Signal::Slot<FetchEmailCommand>	m_clearPartsResponseSlot;
	MojSignal<>::Slot<FetchEmailCommand> 			m_getEmailBodyResponseSlot;
	MojDbClient::Signal::Slot<FetchEmailCommand>	m_updateEmailSummaryResponseSlot;
	MojDbClient::Signal::Slot<FetchEmailCommand>	m_updateEmailPartsResponseSlot;
};

#endif /* FETCHEMAILCOMMAND_H_ */
