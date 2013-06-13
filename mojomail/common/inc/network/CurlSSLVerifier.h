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

#ifndef CURLSSLVERIFIER_H_
#define CURLSSLVERIFIER_H_

#include <curl/curl.h>
#include <string>
#include "network/HttpConnection.h"

class CurlConnection;
class x509_store_ctx_st;

class CurlSSLVerifier
{
public:
	CurlSSLVerifier(CurlConnection* connection);
	virtual ~CurlSSLVerifier();

	void SetupCurlConnection(CURL* easy);

	int GetVerifyResult() { return m_verifyResult; }
	static HttpConnection::ConnectionError GetConnectionError(int verifyResult);

protected:
	static CURLcode SetupSslCtxCallback(CURL* curl, void* sslctx, void* data);
	static int VerifyWrapper(int preverifyOk, x509_store_ctx_st* ctx);

	int Verify(int preverifyOk, x509_store_ctx_st* ctx);

	static bool s_pslInitialized;
	static int s_connectionExDataIndex;

	CurlConnection*		m_curlConnection;
	std::string			m_hostname;

	bool				m_checkedInstalledCert;
	bool				m_hasInstalledCert;

	int					m_verifyResult;
};

#endif /* CURLSSLVERIFIER_H_ */
