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
#include "ImapBusDispatcher.h"
#include "ImapValidator.h"
#include "data/ImapAccount.h"
#include "ImapCoreDefs.h"
#include <sstream>
#include <iostream>
#include "data/DatabaseAdapter.h"
#include "exceptions/ExceptionUtils.h"

using namespace std;

#define CATCH_AS_ERROR \
	catch(const MojErrException& e) { \
		m_msg->replyError(e.GetMojErr(), e.what()); \
		m_listener.ValidationDone(this); \
	} catch(const std::exception& e) { \
		m_msg->replyError(MojErrInternal, e.what()); \
		m_listener.ValidationDone(this); \
	}

MojLogger ImapValidator::s_log("com.palm.mail.validator");

ImapValidator::ImapValidator(ImapValidationListener& listener, MojServiceMessage* msg, MojObject& protocolSettings)
: ImapSession(NULL, ImapValidator::s_log),
  m_listener(listener),
  m_msg(msg),
  m_protocolSettings(protocolSettings),
  m_enableTLS(false),
  m_validationDone(false),
  m_loadPrefsSlot(this, &ImapValidator::LoadPrefsResponse)
{
	// Fake account object
	m_account.reset(new ImapAccount());
}

ImapValidator::~ImapValidator()
{
}

void ImapValidator::Validate()
{
	try {
		if(m_protocolSettings.get("accountId", m_accountId)) {
			// If we're given an account id, load the existing server settings
			LoadImapPrefs();
		} else {
			// If there's no id, the payload must include config settings
			boost::shared_ptr<ImapLoginSettings> loginSettings(new ImapLoginSettings());
			ParseValidatePayload(m_protocolSettings, *loginSettings, true);
			m_account->SetLoginSettings(loginSettings);

			SetAccount(m_account);

			// Start validation process
			CheckQueue();
		}

	} CATCH_AS_ERROR
}

void ImapValidator::LoadImapPrefs()
{
	GetDatabaseInterface().GetAccount(m_loadPrefsSlot, m_accountId);
}

MojErr ImapValidator::LoadPrefsResponse(MojObject& response, MojErr err)
{
	try {
		ErrorToException(err);

		MojObject accountObj;
		DatabaseAdapter::GetOneResult(response, accountObj);

		// Parse server config
		boost::shared_ptr<ImapLoginSettings> loginSettings(new ImapLoginSettings());
		ImapAccountAdapter::ParseLoginConfig(accountObj, *loginSettings);
		m_account->SetLoginSettings(loginSettings);

		// Updates settings with password, new settings, etc.
		ParseValidatePayload(m_protocolSettings, *loginSettings, false);

		SetAccount(m_account);

		// Start validation process
		CheckQueue();
	} CATCH_AS_ERROR

	return MojErrNone;
}

void ImapValidator::ParseValidatePayload(const MojObject& payload, ImapLoginSettings& login, bool serverConfigNeeded)
{
	MojErr err;

	// The account username is the default username and email address
	// Only update the username if one isn't already set.
	if(login.GetUsername().empty()) {
		MojString username;
		err = payload.getRequired("username", username);
		ErrorToException(err);
		login.SetUsername(username.data());
	}

	MojString password;
	err = payload.getRequired("password", password);
	ErrorToException(err);
	login.SetPassword(password.data());

	bool hasTemplateId = false;
	MojString templateId;
	err = payload.get("templateId", templateId, hasTemplateId);
	ErrorToException(err);

	// Load config settings, if any; these override any existing settings
	MojObject config;
	if(payload.get("config", config)) {
		ImapAccountAdapter::ParseLoginConfig(config, login);
	} else if(serverConfigNeeded) {
		// trigger an error
		err = payload.getRequired("config", config);
		ErrorToException(err);

		// just for good measure
		throw MailException("no config provided", __FILE__, __LINE__);
	}
}

bool ImapValidator::NeedLoginCapabilities()
{
	// Should always get login capabilities so we know that LOGINDISABLED means we probably need TLS.
	return true;
}

void ImapValidator::LoginSuccess()
{
	try {
		MojObject reply;
		MojErr err = reply.putBool(MojServiceMessage::ReturnValueKey, true);
		ErrorToException(err);

		// Update encryption settings
		if(GetLoginSettings().GetEncryption() == ImapLoginSettings::Encrypt_TLSIfAvailable) {
			MojObject config;
			if(m_protocolSettings.get("config", config)) {
				err = config.putString(ImapAccountAdapter::ENCRYPTION,
						m_enableTLS ? ImapAccountAdapter::TLS : ImapAccountAdapter::NO_SSL);
				ErrorToException(err);

				err = m_protocolSettings.put("config", config);
				ErrorToException(err);
			}
		}

		err = reply.put("protocolSettings", m_protocolSettings);
		ErrorToException(err);

		// create the credentials object
		// {"common":{"password":"mysecret"}} or
		// {"common":{"securityToken":"myToken"}} for Yahoo

		MojObject common;
		MojObject secret;

		if(m_account->IsYahoo())
		{
			err = secret.putString("securityToken", m_account->GetLoginSettings().GetYahooToken().c_str());
			ErrorToException(err);
		}
		else
		{
			err = secret.putString("password", m_account->GetLoginSettings().GetPassword().c_str());
			ErrorToException(err);
		}

		err = common.put("common", secret);
		ErrorToException(err);
		err = reply.put("credentials", common);
		ErrorToException(err);

		if (m_msg.get()) {
			err = m_msg->reply(reply);
			ErrorToException(err);
		}

		MojLogNotice(m_log, "validation %p succeeded", this);
	} catch (const exception& e) {
		if (m_msg.get()) {
			m_msg->replyError(MojErrInternal);
		}
	}

	// stop the whole state thing here and return the result to the user.
	Disconnect();
	ValidationDone();

	return;
}

/**
 * Handler for login/validation errors.
 */
void ImapValidator::LoginFailure(MailError::ErrorCode errorCode, const std::string& errorText)
{
	Disconnect();

	m_state = State_InvalidCredentials;
	ReportFailure(errorCode, errorText);

	ValidationDone();
}

void ImapValidator::Disconnected()
{
	MojLogInfo(m_log, "validator disconnected");

	// Only if an error hasn't already been reported
	ReportFailure(MailError::CONNECTION_FAILED, "");

	ValidationDone();
}

void ImapValidator::ConnectFailure(const exception& exc)
{
	Disconnect();

	MailError::ErrorInfo errorInfo = ExceptionUtils::GetErrorInfo(exc);
	//fprintf(stderr, "error info: %d, %s\n", errorInfo.errorCode, typeid(exc).name());

	// Force unknown errors to identify as generic connect errors
	if(errorInfo.errorCode == MailError::INTERNAL_ERROR)
		errorInfo.errorCode = MailError::CONNECTION_FAILED;

	ReportFailure(errorInfo.errorCode, errorInfo.errorText);

	ValidationDone();
}

void ImapValidator::TLSReady()
{
	m_enableTLS = true;

	ImapSession::TLSReady();
}

void ImapValidator::TLSFailure(const exception& exc)
{
	MailError::ErrorInfo errorInfo = ExceptionUtils::GetErrorInfo(exc);

	// FIXME should be handled elsewhere
	if(m_capabilities.HasCapability(Capabilities::STARTTLS))
		ReportFailure(errorInfo.errorCode, errorInfo.errorText);
	else
		ReportFailure(MailError::CONFIG_NO_SSL, errorInfo.errorText);

	Disconnect();

	ValidationDone();
}

/**
 * Primary function for error handling. Specific error code handlers should call
 * this function to return error on the bus
 */
void ImapValidator::ReportFailure(MailError::ErrorCode errorCode, const std::string& errorText)
{
	if (!m_msg.get() || m_validationDone) {
		return;
	}

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

	err = m_msg->reply(reply);
	ErrorToException(err);

	MojLogWarning(m_log, "validation %p failed: %d: %s", this, errorCode, errorText.c_str());
}

void ImapValidator::ValidationDone()
{
	if (!m_validationDone) {
		m_validationDone = true;
		m_listener.ValidationDone(this);
	}
}
