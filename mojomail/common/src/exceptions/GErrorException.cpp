// @@@LICENSE
//
//      Copyright (c) 2010-2013 LG Electronics, Inc.
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

#include "exceptions/GErrorException.h"
#include "CommonErrors.h"
#include "palmsocket/palmsockerror.h"
#include <openssl/ssl.h>
#include <openssl/x509v3.h>

GErrorException::GErrorException(const GError* error, const char* filename, int line)
: MailException(error->message, filename, line),
  m_domain(error->domain),
  m_code(error->code)
{
}

void GErrorException::CheckError(GError* error, const char* filename, int line)
{
	if(error != NULL) {
		GErrorException exc = ThrowAndFreeError(error, filename, line);
		throw exc;
	}
}

GErrorException GErrorException::ThrowAndFreeError(GError* error, const char* filename, int line)
{
	GErrorException exc(error, filename, line);
	g_error_free(error);
	return exc;
}

MailError::ErrorInfo GErrorException::GetErrorInfo() const
{
	// GError for GIOChannel only has one useful error code, which is equivalent to CONNECTION_FAILED
	MailError::ErrorInfo errorInfo(MailError::CONNECTION_FAILED, m_msg);

	// FIXME this ought to be done somewhere in GIOChannelWrapper
	if(m_domain == PmSockErrGErrorDomain()) {
		PslErrorException exc( (PslError) m_code, __FILE__, __LINE__);

		return exc.GetErrorInfo();
	}

	return errorInfo;
}

PslErrorException::PslErrorException(PslError error, const char* filename, int line)
: MailException(PmSockErrStringFromError(error), filename, line),
  m_pslError(error), m_sslVerifyResult(0)
{
}

void PslErrorException::SetSSLVerifyResult(int result)
{
	m_sslVerifyResult = result;
}

MailError::ErrorInfo PslErrorException::GetSSLVerifyErrorInfo(int verifyResult)
{
	MailError::ErrorInfo errorInfo(MailError::INTERNAL_ERROR, X509_verify_cert_error_string(verifyResult));

	switch(verifyResult) {
	case X509_V_ERR_CERT_NOT_YET_VALID:
	case X509_V_ERR_CERT_HAS_EXPIRED:
		errorInfo.errorCode = MailError::SSL_CERTIFICATE_EXPIRED;
		break;
	case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		errorInfo.errorCode = MailError::SSL_CERTIFICATE_NOT_TRUSTED;
		break;
	case X509_V_ERR_APPLICATION_VERIFICATION:
		errorInfo.errorCode = MailError::SSL_HOST_NAME_MISMATCHED;
		errorInfo.errorText = "hostname mismatch";
		break;
	default:
		errorInfo.errorCode = MailError::SSL_CERTIFICATE_INVALID;
	}

	return errorInfo;
}

MailError::ErrorInfo PslErrorException::GetErrorInfo() const
{
	MailError::ErrorInfo errorInfo(MailError::INTERNAL_ERROR, m_msg);

	switch(m_pslError) {
	case PSL_ERR_GETADDRINFO:
		errorInfo.errorCode = MailError::HOST_NOT_FOUND;
		break;
	case PSL_ERR_SSL_ALERT_UNKNOWN_CA:
	case PSL_ERR_SSL_CERT_VERIFY:
		// Get more specific SSL error
		errorInfo = GetSSLVerifyErrorInfo(m_sslVerifyResult);
		break;
	case PSL_ERR_SSL_HOSTNAME_MISMATCH:
		errorInfo.errorCode = MailError::SSL_HOST_NAME_MISMATCHED;
		break;
	case PSL_ERR_TCP_CONNECT:
	case PSL_ERR_TCP_CONNREFUSED:
	case PSL_ERR_TCP_CONNRESET:
	case PSL_ERR_TCP_NETUNREACH:
	case PSL_ERR_TIMEDOUT:
		errorInfo.errorCode = MailError::CONNECTION_FAILED;
		break;
	case PSL_ERR_SSL_PROTOCOL:
		// Most likely the "encryption" setting is wrong
		errorInfo.errorCode = MailError::CONFIG_NO_SSL;
		break;
	case PSL_ERR_BAD_BIND_ADDR:
		errorInfo.errorCode = MailError::INTERNAL_ERROR;
		break;
	default:
		break;
	}

	return errorInfo;
}
