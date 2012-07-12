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

#include "PopErrors.h"

bool PopErrors::IsLoginError(EmailAccount::AccountError err)
{
	return err.errorCode == MailError::ACCOUNT_LOCKED
				|| err.errorCode == MailError::ACCOUNT_WEB_LOGIN_REQUIRED
				|| err.errorCode == MailError::ACCOUNT_UNKNOWN_AUTH_ERROR
				|| err.errorCode == MailError::BAD_USERNAME_OR_PASSWORD;
}

bool PopErrors::IsRetryNetworkError(EmailAccount::AccountError err)
{
	return err.errorCode == MailError::CONNECTION_FAILED
			|| err.errorCode == MailError::CONNECTION_TIMED_OUT
			|| err.errorCode == MailError::HOST_NOT_FOUND
			|| err.errorCode == MailError::NO_NETWORK;
}

bool PopErrors::IsSSLNetworkError(EmailAccount::AccountError err)
{
	return err.errorCode == MailError::SSL_CERTIFICATE_EXPIRED
			|| err.errorCode == MailError::SSL_CERTIFICATE_INVALID
			|| err.errorCode == MailError::SSL_CERTIFICATE_NOT_TRUSTED
			|| err.errorCode == MailError::SSL_HOST_NAME_MISMATCHED;
}

bool PopErrors::IsNetworkError(EmailAccount::AccountError err)
{
	return IsRetryNetworkError(err) || IsSSLNetworkError(err);
}

bool PopErrors::IsRetryAccountError(EmailAccount::AccountError err)
{
	// Hotmail, Live, or MSN account can has temporary account unavailable
	// error, which means that the account is temporarily locked due to too
	// frequent account access.  In this case of error, we can schedule a sync
	// to happen in 15-30 minutes.
	return err.errorCode == MailError::ACCOUNT_UNAVAILABLE;
}

bool PopErrors::IsRetryError(EmailAccount::AccountError err)
{
	return IsRetryNetworkError(err) || IsRetryAccountError(err);
}
