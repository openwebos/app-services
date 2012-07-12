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

#include "network/CurlSSLVerifier.h"
#include <openssl/ssl.h>
#include "exceptions/CurlException.h"
#include "network/CurlConnection.h"
#include "palmsockopensslutils.h"

using namespace std;

bool CurlSSLVerifier::s_pslInitialized = false;
int CurlSSLVerifier::s_connectionExDataIndex = -1; // must be < 0 to indicate that it hasn't been initialized

CurlSSLVerifier::CurlSSLVerifier(CurlConnection* connection)
: m_curlConnection(connection),
  m_checkedInstalledCert(false),
  m_hasInstalledCert(false),
  m_verifyResult(0)
{
	if(!s_pslInitialized) {
		PmSockOpensslInit(kPmSockOpensslInitType_DEFAULT);
		s_pslInitialized = true;
	}
}

CurlSSLVerifier::~CurlSSLVerifier()
{
}

void CurlSSLVerifier::SetupCurlConnection(CURL* easy)
{
	CURLcode curlErr;

	// Setup callback to configure SSL context
	curlErr = curl_easy_setopt(easy, CURLOPT_SSL_CTX_FUNCTION, &CurlSSLVerifier::SetupSslCtxCallback);
	if(curlErr)
		throw CurlException(curlErr, __FILE__, __LINE__);

	curlErr = curl_easy_setopt(easy, CURLOPT_SSL_CTX_DATA, m_curlConnection);
	if(curlErr)
		throw CurlException(curlErr, __FILE__, __LINE__);

	// Disable hostname validation; we'll do it ourselves
	curlErr = curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0);
	if(curlErr)
		throw CurlException(curlErr, __FILE__, __LINE__);
}

CURLcode CurlSSLVerifier::SetupSslCtxCallback(CURL *curl, void *sslCtx, void *data)
{
	// Clear state. This should get done once per connection.
	CurlConnection* connection = reinterpret_cast<CurlConnection*>(data);
	if(connection) {
		CurlSSLVerifier& verifier = connection->GetSSLVerifier();
		verifier.m_hostname = "";
		verifier.m_checkedInstalledCert = false;
		verifier.m_hasInstalledCert = false;
		verifier.m_verifyResult = 0;
	}

	// Setup ex data index to stuff our pointer
	if(s_connectionExDataIndex < 0) // initialize; note, zero is a valid index
		s_connectionExDataIndex = SSL_CTX_get_ex_new_index(0, (void*) "CurlConnection", NULL, NULL, NULL);

	SSL_CTX_set_ex_data( (SSL_CTX*) sslCtx, s_connectionExDataIndex, data);

	SSL_CTX_set_verify( (SSL_CTX*) sslCtx, SSL_VERIFY_PEER, &CurlSSLVerifier::VerifyWrapper);

	return CURLE_OK;
}

// Gets SSLVerifier instance and calls Verify
int CurlSSLVerifier::VerifyWrapper(int preverifyOk, X509_STORE_CTX* storeCtx)
{
	try {
		// Get the SSL object from the X509_STORE_CTX
		SSL* ssl = (SSL*) X509_STORE_CTX_get_ex_data(storeCtx, SSL_get_ex_data_X509_STORE_CTX_idx());
		if(!ssl)
			throw MailException("no SSL object available", __FILE__, __LINE__);

		// Get the SSL_CTX from the SSL object
		SSL_CTX* sslCtx = SSL_get_SSL_CTX(ssl);
		if(!sslCtx)
			throw MailException("no SSL object available", __FILE__, __LINE__);

		void* data = SSL_CTX_get_ex_data(sslCtx, s_connectionExDataIndex);

		if(data) {
			CurlConnection* connection = reinterpret_cast<CurlConnection*>(data);

			if(connection) {
				CurlSSLVerifier& verifier = connection->GetSSLVerifier();

				// Update hostname with current URL
				if(verifier.m_hostname.empty()) {
					verifier.m_hostname = connection->GetCurrentHostname();
				}

				return verifier.Verify(preverifyOk, storeCtx);
			} else {
				throw MailException("no CurlConnection object available", __FILE__, __LINE__);
			}
		}

	} catch(...) {
		return false;
	}

	return preverifyOk;
}

// Adapted from libpalmsocket crypto_ssl_peer_verify_callback
int CurlSSLVerifier::Verify(int preverifyOk, X509_STORE_CTX* storeCtx)
{
	int verifyDepth = X509_STORE_CTX_get_error_depth(storeCtx);
	int certErr = X509_STORE_CTX_get_error(storeCtx);
	X509* curCert = X509_STORE_CTX_get_current_cert(storeCtx);
	bool isPeerCertDepth = (0 == verifyDepth);

	if (!preverifyOk) {
		switch (certErr) {
		case X509_V_ERR_UNABLE_TO_GET_CRL: ///< missing CRL extension
			preverifyOk = true;
			break;
		default:
			break;
		}
	}

	//fprintf(stderr, "verifying certificate, hostname = %s\n", m_hostname.c_str());

	// Check hostname
	if (preverifyOk && isPeerCertDepth) {
		bool matched = false;

		PslError pslError = PmSockOpensslVerifyHostname(
				m_hostname.c_str(),
				curCert,
				0/*verifyOpts*/,
				0/*nameMatchOpts*/,
				&matched);

		if(pslError || !matched) {
			// We use this error code to indicate a hostname mismatch
			certErr = X509_V_ERR_APPLICATION_VERIFICATION;
			X509_STORE_CTX_set_error(storeCtx, certErr);
			preverifyOk = false;
		}
	}

	if(!preverifyOk) {
		if(!m_checkedInstalledCert) {
			bool foundMatch = false;

			// Check if a matching leaf certificate exists on the device
			PslError pslErr = PmSockOpensslMatchCertInStore(
					storeCtx, storeCtx->cert, 0/*opts*/, &foundMatch);

			m_checkedInstalledCert = true;

			if (!pslErr && foundMatch) {
				// All further certificates are preverified now
				m_hasInstalledCert = true;
			}
		}

		if(m_hasInstalledCert) {
			preverifyOk = true;
		}
	}

	// Record last error so we can display an appropriate message to the user
	m_verifyResult = certErr;

	if(isPeerCertDepth) {
		if(m_hasInstalledCert) {
			// clear error
			X509_STORE_CTX_set_error(storeCtx, X509_V_OK);
		}

		// Cleanup
		m_checkedInstalledCert = false;
		m_hasInstalledCert = false;
	}

	return preverifyOk;
}

HttpConnection::ConnectionError CurlSSLVerifier::GetConnectionError(int verifyResult)
{
	switch(verifyResult) {
	case X509_V_ERR_CERT_NOT_YET_VALID:
	case X509_V_ERR_CERT_HAS_EXPIRED:
		return HttpConnection::HTTP_ERROR_SSL_CERTIFICATE_EXPIRED;
	case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
		return HttpConnection::HTTP_ERROR_SSL_CERTIFICATE_NOT_TRUSTED;
	case X509_V_ERR_APPLICATION_VERIFICATION:
		return HttpConnection::HTTP_ERROR_SSL_HOST_NAME_MISMATCHED;
	default:
		return HttpConnection::HTTP_ERROR_SSL_CERTIFICATE_INVALID;
	}
}
