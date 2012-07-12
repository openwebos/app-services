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

#include "network/HttpConnection.h"

const char* const HttpConnection::CONTENT_LENGTH		 	= "Content-Length";
const char* const HttpConnection::CONTENT_TYPE			 	= "Content-Type";
const char* const HttpConnection::CONTENT_TYPE_WBXML		= "Content-Type: application/vnd.ms-sync.wbxml";
const char* const HttpConnection::CONTENT_TYPE_RFC822	 	= "Content-Type: message/rfc822";
const char* const HttpConnection::EXPECT				 	= "Expect";
const char* const HttpConnection::HEADER_POLICY_KEY			= "X-MS-PolicyKey: ";
const char* const HttpConnection::HEADER_PROTOCOL_VERSION 	= "MS-ASProtocolVersion: ";
const char* const HttpConnection::HEADER_USER_AGENT 		= "User-Agent: ";

HttpConnection::HttpConnection()
: m_complete(false)
{

}

HttpConnection::~HttpConnection()
{

}

MailError::ErrorCode HttpConnection::GetMailErrorCode(ConnectionError err)
{
	MailError::ErrorCode errorCode;

	switch(err) {
		case HttpConnection::HTTP_ERROR_COULDNT_RESOLVE_PROXY:
		case HttpConnection::HTTP_ERROR_COULDNT_RESOLVE_HOST:
			errorCode = MailError::HOST_NOT_FOUND;
			break;
		case HttpConnection::HTTP_ERROR_COULDNT_CONNECT:
			errorCode = MailError::CONNECTION_FAILED;
			break;
		case HttpConnection::HTTP_ERROR_OPERATION_TIMEDOUT:
			errorCode = MailError::CONNECTION_TIMED_OUT;
			break;
		case HttpConnection::HTTP_ERROR_SSL_CERTIFICATE_EXPIRED:
			errorCode = MailError::SSL_CERTIFICATE_EXPIRED;
			break;
		case HttpConnection::HTTP_ERROR_SSL_CERTIFICATE_NOT_TRUSTED:
			errorCode = MailError::SSL_CERTIFICATE_NOT_TRUSTED;
			break;
		case HttpConnection::HTTP_ERROR_SSL_HOST_NAME_MISMATCHED:
			errorCode = MailError::SSL_HOST_NAME_MISMATCHED;
			break;
		case HttpConnection::HTTP_ERROR_SSL_CERTIFICATE_INVALID:
			errorCode = MailError::SSL_CERTIFICATE_INVALID;
			break;
		case HttpConnection::HTTP_ERROR_PEER_FAILED_VERIFICATION:
		case HttpConnection::HTTP_ERROR_SSL_ENGINE_NOTFOUND:
		case HttpConnection::HTTP_ERROR_SSL_ENGINE_SETFAILED:
		case HttpConnection::HTTP_ERROR_SSL_CERTPROBLEM:
		case HttpConnection::HTTP_ERROR_SSL_CIPHER:
		case HttpConnection::HTTP_ERROR_SSL_CACERT:
		case HttpConnection::HTTP_ERROR_USE_SSL_FAILED:
		case HttpConnection::HTTP_ERROR_SSL_ENGINE_INITFAILED:
		case HttpConnection::HTTP_ERROR_SSL_CACERT_BADFILE:
		case HttpConnection::HTTP_ERROR_SSL_SHUTDOWN_FAILED:
		case HttpConnection::HTTP_ERROR_SSL_CRL_BADFILE:
		case HttpConnection::HTTP_ERROR_SSL_ISSUER_ERROR:
			errorCode = MailError::SSL_CERTIFICATE_INVALID;
			break;
		default:
			errorCode = MailError::CONNECTION_FAILED;
	}

	return errorCode;
}

MailError::ErrorCode HttpConnection::GetHttpMailErrorCode(long httpResponseCode)
{
	switch (httpResponseCode) {
	case HttpConnection::HTTP_OK:
		return MailError::INTERNAL_ERROR;
	case HttpConnection::HTTP_NOT_FOUND:
		return MailError::EMAIL_NOT_FOUND;
	case HttpConnection::HTTP_TOO_LARGE:
		return MailError::ATTACHMENT_TOO_LARGE;
	case HttpConnection::HTTP_UNAUTHORIZED:
		return MailError::BAD_USERNAME_OR_PASSWORD;
	case HttpConnection::HTTP_FORBIDDEN:
		return MailError::ACCOUNT_NEEDS_PROVISIONING;
	default:
		return MailError::EAS_SERVER_ERROR;
	}
}

MailError::ErrorCode HttpConnection::GetMailErrorCode(HttpConnection::ConnectionError connectionError, long httpResponseCode)
{
	if (connectionError != HttpConnection::HTTP_ERROR_NONE) {
		return GetMailErrorCode(connectionError);
	} else {
		return GetHttpMailErrorCode(httpResponseCode);
	}
}
