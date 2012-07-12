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

#ifndef CURLCONNECTION_H_
#define CURLCONNECTION_H_

#include "curl/curl.h"
#include "glibcurl/glibcurl.h"
#include "exceptions/CurlException.h"
#include "network/HttpConnection.h"
#include <string>
#include "network/CurlSSLVerifier.h"

/**
 * \brief
 * A CurlConnection is a C++ wrapper for a libcurl easy_handle.
 * @see http://curl.haxx.se/libcurl/
 */
class CurlConnection : public HttpConnection
{
public:
	static int created;
	static int deleted;

	CurlConnection();
	virtual ~CurlConnection();

	/**
	 * Method called if SetStreamingPost() was called.
	 * This method is called to populate the buffer to be sent to the server during POST.
	 * @param the byte buffer
	 * @param the maximum size of the buffer
	 * @return the number of bytes stored in the buffer
	 */
	virtual size_t ReadData(char* buffer, size_t size) = 0;

	/**
	 * This method is called whenever data is received from the server.
	 * @param the byte buffer
	 * @param the number of bytes to read from the buffer
	 * @return the number of bytes read/processed (should always equal length or an error will occur)
	 */
	virtual size_t WriteData(char* buffer, size_t length) = 0;

	/**
	 * This method is called whenever a header is received from the server
	 */
	virtual void Header(const std::string& header) { };

	/**
	 * Method called if an exception is thrown while reading/writing data.
	 * @param the exception thrown
	 */
	virtual void ExceptionThrown(const std::exception& e) = 0;

	/**
	 * Called when the connection has sent and received all data (or encountered an error).
	 */
	virtual void ConnectionDone(HttpConnection::ConnectionError err) = 0;

	// HttpConnection Methods
	virtual void SetMethod(ConnectionMethod method);
	virtual void SetHeader(const std::string& header);
	virtual void SetCredentials(const std::string& username, const std::string& password);
	virtual void SetUrl(const std::string& url);
	virtual std::string GetUrl() const;
	virtual void SetTimeout(long seconds);
	virtual void SetPostData(void* data, size_t size);
	virtual void SetStreamingPost(size_t size);
	virtual void SetEmptyPost();
	virtual void DisableConnectionReuse();
	virtual void Connect();
	virtual long GetResponseCode();
	virtual std::string UrlEncode(const std::string& str);
	virtual void Pause();
	virtual void Resume();
	virtual bool IsPaused() const { return m_paused; }
	virtual size_t MaxWriteBufferSize() { return CURL_MAX_WRITE_SIZE; }

	virtual std::string GetCurrentHostname();
	virtual CurlSSLVerifier& GetSSLVerifier();

protected:
	static HttpConnection::ConnectionError GetErrorCode(CURLcode code);

	static std::string ExtractHostnameFromUrl(const std::string& url);

protected:
	/**
	 * Time in seconds to wait for the connection to be established.
	 */
	static const long CONNECT_TIMEOUT;

	/**
	 * The minimum transfer rate in bytes per second allowed before the connection will error out.
	 */
	static const long LOW_SPEED_LIMIT;

	/**
	 * The minimum duration in seconds the connection must be <= LOW_SPEED_LIMIT before it errors out.
	 */
	static const long LOW_SPEED_TIME;

	/**
	 * Callback function used by libcurl which is called whenever it needs to read data from a HTTP connection.
	 * @see http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTREADFUNCTION
	 */
	static size_t CurlWriteCallback(char* stream, size_t size, size_t nmemb, CurlConnection* connection);
	
	/**
	 * Callback function used by libcurl which is called whenever it needs to write data to a HTTP connection.
	 * @see http://curl.haxx.se/libcurl/c/curl_easy_setopt.html#CURLOPTREADFUNCTION
	 */
	static size_t CurlReadCallback(char* stream, size_t size, size_t nmemb, CurlConnection* connection);

	/**
	 * Callback function used by libcurl which is called whenever a complete HTTP header is received.
	 */
	static size_t CurlHeaderCallback(char* stream, size_t size, size_t nmemb, CurlConnection* connection);

	/**
	 * glibcurl multi_perform callback
	 */
	static void GlibcurlCallback(void* data);

	CURL*				m_easyHandle;
	std::string 		m_url;
	std::string 		m_userpwd;
	struct curl_slist* 	m_headers;
	bool				m_connected;
	bool				m_paused;
	long				m_totalOperationTimeout;

	CurlSSLVerifier		m_sslVerifier;
};

#endif /* CURLCONNECTION_H_ */
