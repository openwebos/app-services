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

#ifndef IMAPVALIDATOR_H_
#define IMAPVALIDATOR_H_

#include "client/ImapSession.h"
#include "ImapValidationListener.h"
#include "ImapConfig.h"
#include "CommonErrors.h"

class ImapAccount;

class ImapValidator : public ImapSession
{
public:
	ImapValidator(ImapValidationListener& listener, MojServiceMessage* msg, MojObject& protocolSettings);
	virtual ~ImapValidator();

	void Validate();
	virtual bool IsValidator() const { return true; }

protected:
	// Overrides ImapSession
	void ConnectFailure(const std::exception& e);
	void TLSReady();
	void TLSFailure(const std::exception& e);
	void LoginSuccess();
	void LoginFailure(MailError::ErrorCode errorCode, const std::string& errorText);

	void Disconnected();

	void LoadImapPrefs();
	MojErr LoadPrefsResponse(MojObject& response, MojErr err);

	void ParseValidatePayload(const MojObject& config, ImapLoginSettings& login, bool serverConfigNeeded);

	bool NeedLoginCapabilities();

	void ReportFailure(MailError::ErrorCode errorCode, const std::string& errorText);

	void ValidationDone();

	static MojLogger					s_log;

	ImapValidationListener&				m_listener;
	MojRefCountedPtr<MojServiceMessage> m_msg;
	MojObject							m_protocolSettings;
	MojObject							m_accountId;
	bool								m_enableTLS;
	bool								m_validationDone;

	MojDbClient::Signal::Slot<ImapValidator>	m_loadPrefsSlot;
};

#endif /* IMAPVALIDATOR_H_ */
