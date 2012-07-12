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

#include "CommonErrors.h"

using namespace MailError;

std::string MailError::GetAccountErrorCode(MailError::ErrorCode mailErrorCode)
{
	switch (mailErrorCode) {
	case BAD_USERNAME_OR_PASSWORD:
	case ACCOUNT_UNKNOWN_AUTH_ERROR:
	case LOGIN_TIMEOUT:
										return "401_UNAUTHORIZED";
	case MISSING_CREDENTIALS:			return "CREDENTIALS_NOT_FOUND";
	case EAS_CONFIG_BAD_URL:			return "412_PRECONDITION_FAILED";
	case HOST_NOT_FOUND: 				return "HOST_NOT_FOUND";
	case CONNECTION_TIMED_OUT:			return "CONNECTION_TIMEOUT";
	case CONNECTION_FAILED:				return "CONNECTION_FAILED";
	case NO_NETWORK:					return "NO_CONNECTIVITY";
	case SSL_CERTIFICATE_EXPIRED:		return "SSL_CERT_EXPIRED";
	case SSL_CERTIFICATE_NOT_TRUSTED:	return "SSL_CERT_UNTRUSTED";
	case SSL_CERTIFICATE_INVALID:		return "SSL_CERT_INVALID";
	case SSL_HOST_NAME_MISMATCHED:		return "SSL_CERT_HOSTNAME_MISMATCH";
	default:							return "UNKNOWN_ERROR";
	};
}

bool MailError::IsConnectionError(MailError::ErrorCode mailErrorCode)
{
	switch(mailErrorCode) {
	case HOST_NOT_FOUND:
	case CONNECTION_TIMED_OUT:
	case CONNECTION_FAILED:
	case NO_NETWORK:
		return true;
	default:
		return false;
	}
}

bool MailError::IsSSLError(MailError::ErrorCode mailErrorCode)
{
	switch(mailErrorCode) {
	case SSL_CERTIFICATE_EXPIRED:
	case SSL_CERTIFICATE_NOT_TRUSTED:
	case SSL_CERTIFICATE_INVALID:
	case SSL_HOST_NAME_MISMATCHED:
		return true;
	default:
		return false;
	}
}
