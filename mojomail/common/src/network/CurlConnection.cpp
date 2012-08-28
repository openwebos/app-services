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

#include "network/CurlConnection.h"
#include "exceptions/MailException.h"
#include "network/CurlSSLVerifier.h"

#define CheckError(err) if (err) throw CurlException(err, __FILE__, __LINE__);
#define UnknownException MailException(__PRETTY_FUNCTION__, __FILE__, __LINE__)

using namespace std;

int CurlConnection::created = 0;
int CurlConnection::deleted = 0;

const long CurlConnection::CONNECT_TIMEOUT 	= 60; 	// 1 minute
const long CurlConnection::LOW_SPEED_LIMIT 	= 10; 	// 10 bps (stolen from Download Manager)
const long CurlConnection::LOW_SPEED_TIME	= 120;	// 2 minutes (some EAS servers take very long to respond)

CurlConnection::CurlConnection()
: m_easyHandle(NULL),
  m_headers(NULL),
  m_connected(false),
  m_paused(false),
  m_totalOperationTimeout(0),
  m_sslVerifier(this)
{
	m_easyHandle = curl_easy_init();
	if (!m_easyHandle)
		throw MailException("Failed to create easy handle", __FILE__, __LINE__);
	created++;
}

CurlConnection::~CurlConnection()
{
	if (m_easyHandle) {
		// We should only call glibcurl_remove if glibcurl_add has been called
		if (m_connected)
			glibcurl_remove(m_easyHandle);

		curl_easy_cleanup(m_easyHandle);
		m_easyHandle = NULL;
		deleted++;
	}

	if (m_headers) {
		curl_slist_free_all(m_headers);
		m_headers = NULL;
	}
}

void CurlConnection::SetMethod(ConnectionMethod method)
{
	switch(method)
	{
		case MethodPost:
		{
			CURLcode err = curl_easy_setopt(m_easyHandle, CURLOPT_POST, 1);
			CheckError(err);
			break;
		}
		case MethodGet:
		{
			break;
		}
		case MethodOptions:
		{
			CURLcode err = curl_easy_setopt(m_easyHandle, CURLOPT_CUSTOMREQUEST, "OPTIONS");
			CheckError(err);
			break;
		}
		default:
			assert(false);
	}
}

void CurlConnection::SetHeader(const string& header)
{
	m_headers = curl_slist_append(m_headers, header.c_str());
	CURLcode err = curl_easy_setopt(m_easyHandle, CURLOPT_HTTPHEADER, m_headers);
	CheckError(err);
}

void CurlConnection::SetCredentials(const string& username, const string& password)
{
	m_userpwd = username;
	m_userpwd += ":";
	m_userpwd += password;
}

void CurlConnection::SetUrl(const string& url)
{
	m_url = url;
}

string CurlConnection::GetUrl() const
{
	return m_url;
}

void CurlConnection::SetTimeout(long seconds)
{
	m_totalOperationTimeout = seconds;
}

void CurlConnection::SetPostData(void* data, size_t size)
{
	CURLcode err = curl_easy_setopt(m_easyHandle, CURLOPT_POSTFIELDSIZE, size);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_POSTFIELDS, data);
	CheckError(err);
}

void CurlConnection::SetStreamingPost(size_t size)
{
	CURLcode err = curl_easy_setopt(m_easyHandle, CURLOPT_POST, 1);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_POSTFIELDSIZE, size);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_READFUNCTION, &CurlReadCallback);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_READDATA, this);
	CheckError(err);
}

void CurlConnection::SetEmptyPost()
{
	// Set no post data to be sent
	CURLcode err = curl_easy_setopt(m_easyHandle, CURLOPT_POST, 1);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_POSTFIELDSIZE, 0);
	CheckError(err);

	// Remove the default "Content-Type: application/x-www-form-urlencoded" header libcurl adds
	string contentType(HttpConnection::CONTENT_TYPE);
	contentType.append(":");
	SetHeader(contentType);
}

void CurlConnection::DisableConnectionReuse()
{
	CURLcode err = curl_easy_setopt(m_easyHandle, CURLOPT_FORBID_REUSE, 1);
	CheckError(err);
}

void CurlConnection::Connect()
{
	// Set the URL
	CURLcode err = curl_easy_setopt(m_easyHandle, CURLOPT_URL, m_url.c_str());
	CheckError(err);

	// Set the username:password (if it has one)
	if (! m_userpwd.empty()) {
		err = curl_easy_setopt(m_easyHandle, CURLOPT_USERPWD, m_userpwd.c_str());
		CheckError(err);
	}

	// apparently this is a good idea especially in multi-threaded environments
    err = curl_easy_setopt(m_easyHandle, CURLOPT_NOSIGNAL, 1L);
    CheckError(err);

    err = curl_easy_setopt(m_easyHandle, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT);
    CheckError(err);

    if (m_totalOperationTimeout > 0) {
    	err = curl_easy_setopt(m_easyHandle, CURLOPT_TIMEOUT, m_totalOperationTimeout);
    	CheckError(err);
    } else {
        err = curl_easy_setopt(m_easyHandle, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);
        CheckError(err);

    	err= curl_easy_setopt(m_easyHandle, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
    	CheckError(err);
    }

	err = curl_easy_setopt(m_easyHandle, CURLOPT_FOLLOWLOCATION, 1);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_PRIVATE, this);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_WRITEFUNCTION, &CurlWriteCallback);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_WRITEDATA, this);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_HEADERFUNCTION, &CurlHeaderCallback);
	CheckError(err);

	err = curl_easy_setopt(m_easyHandle, CURLOPT_HEADERDATA, this);
	CheckError(err);

	// Remove the default "Expect: 100-continue" header libcurl adds
	string expect(HttpConnection::EXPECT);
	expect.append(":");
	SetHeader(expect);

	// Setup SSL verification
	m_sslVerifier.SetupCurlConnection(m_easyHandle);

	// Set the multi_handle_perform callback
	glibcurl_set_callback(&GlibcurlCallback, NULL);

	// Add the handle to the event loop
	CURLMcode mErr = glibcurl_add(m_easyHandle);
	CheckError(mErr);

	m_connected = true;
}

long CurlConnection::GetResponseCode()
{
	long httpResponseCode = 0;
	CURLcode err = curl_easy_getinfo(m_easyHandle, CURLINFO_RESPONSE_CODE, &httpResponseCode);
	CheckError(err);
	return httpResponseCode;
}

std::string CurlConnection::UrlEncode(const std::string& str)
{
	if (m_easyHandle) {
		char* encoded = curl_easy_escape(m_easyHandle, str.c_str() , str.length());
		if (encoded) {
			string encodedStr(encoded);
			curl_free(encoded);
			return encodedStr;
		}
	}
	throw MailException("Failed to URL encode string.", __FILE__, __LINE__);
}

void CurlConnection::Pause()
{
	if (!m_paused) {
		assert(m_easyHandle);
		m_paused = true;
		CURLcode err = curl_easy_pause(m_easyHandle, CURLPAUSE_ALL);
		CheckError(err);
	}
}

void CurlConnection::Resume()
{
	if (m_paused) {
		assert(m_easyHandle);
		m_paused = false;
		CURLcode err = curl_easy_pause(m_easyHandle, CURLPAUSE_CONT);
		CheckError(err);
	}
}

size_t CurlConnection::CurlWriteCallback(char* stream, size_t size, size_t nmemb, CurlConnection* connection)
{
	try {
		size_t bytesWritten = connection->WriteData(stream, (size * nmemb));
		if (!bytesWritten && connection->m_paused)
			return CURL_WRITEFUNC_PAUSE;
		else
			return bytesWritten;
	} catch (const exception& e) {
		connection->ExceptionThrown(e);
	} catch (...) {
		connection->ExceptionThrown(UnknownException);
	}

	return (size_t) -1;
}

size_t CurlConnection::CurlReadCallback(char* stream, size_t size, size_t nmemb, CurlConnection* connection)
{
	try {
		size_t bytesRead = connection->ReadData(stream, (size * nmemb));
		if (connection->m_paused)
			return CURL_READFUNC_PAUSE;
		else
			return bytesRead;
	} catch (const exception& e) {
		connection->ExceptionThrown(e);
	} catch (...) {
		connection->ExceptionThrown(UnknownException);
	}

	return (size_t) -1;
}

size_t CurlConnection::CurlHeaderCallback(char* stream, size_t size, size_t nmemb, CurlConnection* connection)
{
	try {
		size_t headerSize = size * nmemb;
		string header(stream, headerSize);
		connection->Header(header);
		return headerSize;
	} catch (const exception& e) {
		connection->ExceptionThrown(e);
	} catch (...) {
		connection->ExceptionThrown(UnknownException);
	}

	return (size_t) -1;
}

void CurlConnection::GlibcurlCallback(void* data)
{
	// Loop through the message queue to see if we're done with the connection
	CURLMsg* msg = NULL;
	int messagesInQueue = -1;
	while (1) {
		msg = curl_multi_info_read(glibcurl_handle(), &messagesInQueue);
		if (!msg) break;

		if (msg->msg == CURLMSG_DONE) {
			CurlConnection* connection = NULL;
			curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &connection);

			try {
				ConnectionError error;

				switch(msg->data.result) {
					case CURLE_SSL_CACERT:
					case CURLE_PEER_FAILED_VERIFICATION:
					case CURLE_SSL_ISSUER_ERROR:
					{
						// Try to get a more specific SSL error
						int verifyResult = connection->m_sslVerifier.GetVerifyResult();
						error = CurlSSLVerifier::GetConnectionError(verifyResult);
						break;
					}
					default:
						error = GetErrorCode(msg->data.result);
				}

				connection->m_complete = true;
				connection->ConnectionDone(error);
			} catch (const exception& e) {
				connection->ExceptionThrown(e);
			} catch (...) {
				connection->ExceptionThrown(UnknownException);
			}
		}
	}
}

HttpConnection::ConnectionError CurlConnection::GetErrorCode(CURLcode code)
{
	switch (code) {
		case CURLE_OK:
			return HTTP_ERROR_NONE;
		case CURLE_UNSUPPORTED_PROTOCOL:
			return HTTP_ERROR_UNSUPPORTED_PROTOCOL;
		case CURLE_FAILED_INIT:
			return HTTP_ERROR_FAILED_INIT;
		case CURLE_URL_MALFORMAT:
			return HTTP_ERROR_URL_MALFORMAT;
		case CURLE_NOT_BUILT_IN:
			return HTTP_ERROR_OBSOLETE4;
		case CURLE_COULDNT_RESOLVE_PROXY:
			return HTTP_ERROR_COULDNT_RESOLVE_PROXY;
		case CURLE_COULDNT_RESOLVE_HOST:
			return HTTP_ERROR_COULDNT_RESOLVE_HOST;
		case CURLE_COULDNT_CONNECT:
			return HTTP_ERROR_COULDNT_CONNECT;
		case CURLE_FTP_WEIRD_SERVER_REPLY:
			return HTTP_ERROR_FTP_WEIRD_SERVER_REPLY;
		case CURLE_REMOTE_ACCESS_DENIED:
			return HTTP_ERROR_REMOTE_ACCESS_DENIED;
		case CURLE_OBSOLETE10:
			return HTTP_ERROR_OBSOLETE10;
		case CURLE_FTP_WEIRD_PASS_REPLY:
			return HTTP_ERROR_FTP_WEIRD_PASS_REPLY;
		case CURLE_OBSOLETE12:
			return HTTP_ERROR_OBSOLETE12;
		case CURLE_FTP_WEIRD_PASV_REPLY:
			return HTTP_ERROR_FTP_WEIRD_PASV_REPLY;
		case CURLE_FTP_WEIRD_227_FORMAT:
			return HTTP_ERROR_FTP_WEIRD_227_FORMAT;
		case CURLE_FTP_CANT_GET_HOST:
			return HTTP_ERROR_FTP_CANT_GET_HOST;
		case CURLE_OBSOLETE16:
			return HTTP_ERROR_OBSOLETE16;
		case CURLE_FTP_COULDNT_SET_TYPE:
			return HTTP_ERROR_FTP_COULDNT_SET_TYPE;
		case CURLE_PARTIAL_FILE:
			return HTTP_ERROR_PARTIAL_FILE;
		case CURLE_FTP_COULDNT_RETR_FILE:
			return HTTP_ERROR_FTP_COULDNT_RETR_FILE;
		case CURLE_OBSOLETE20:
			return HTTP_ERROR_OBSOLETE20;
		case CURLE_QUOTE_ERROR:
			return HTTP_ERROR_QUOTE_ERROR;
		case CURLE_HTTP_RETURNED_ERROR:
			return HTTP_ERROR_HTTP_RETURNED_ERROR;
		case CURLE_WRITE_ERROR:
			return HTTP_ERROR_WRITE_ERROR;
		case CURLE_OBSOLETE24:
			return HTTP_ERROR_OBSOLETE24;
		case CURLE_UPLOAD_FAILED:
			return HTTP_ERROR_UPLOAD_FAILED;
		case CURLE_READ_ERROR:
			return HTTP_ERROR_READ_ERROR;
		case CURLE_OUT_OF_MEMORY:
			return HTTP_ERROR_OUT_OF_MEMORY;
		case CURLE_OPERATION_TIMEDOUT:
			return HTTP_ERROR_OPERATION_TIMEDOUT;
		case CURLE_OBSOLETE29:
			return HTTP_ERROR_OBSOLETE29;
		case CURLE_FTP_PORT_FAILED:
			return HTTP_ERROR_FTP_PORT_FAILED;
		case CURLE_FTP_COULDNT_USE_REST:
			return HTTP_ERROR_FTP_COULDNT_USE_REST;
		case CURLE_OBSOLETE32:
			return HTTP_ERROR_OBSOLETE32;
		case CURLE_RANGE_ERROR:
			return HTTP_ERROR_RANGE_ERROR;
		case CURLE_HTTP_POST_ERROR:
			return HTTP_ERROR_HTTP_POST_ERROR;
		case CURLE_SSL_CONNECT_ERROR:
			return HTTP_ERROR_SSL_CONNECT_ERROR;
		case CURLE_BAD_DOWNLOAD_RESUME:
			return HTTP_ERROR_BAD_DOWNLOAD_RESUME;
		case CURLE_FILE_COULDNT_READ_FILE:
			return HTTP_ERROR_FILE_COULDNT_READ_FILE;
		case CURLE_LDAP_CANNOT_BIND:
			return HTTP_ERROR_LDAP_CANNOT_BIND;
		case CURLE_LDAP_SEARCH_FAILED:
			return HTTP_ERROR_LDAP_SEARCH_FAILED;
		case CURLE_OBSOLETE40:
			return HTTP_ERROR_OBSOLETE40;
		case CURLE_FUNCTION_NOT_FOUND:
			return HTTP_ERROR_FUNCTION_NOT_FOUND;
		case CURLE_ABORTED_BY_CALLBACK:
			return HTTP_ERROR_ABORTED_BY_CALLBACK;
		case CURLE_BAD_FUNCTION_ARGUMENT:
			return HTTP_ERROR_BAD_FUNCTION_ARGUMENT;
		case CURLE_OBSOLETE44:
			return HTTP_ERROR_OBSOLETE44;
		case CURLE_INTERFACE_FAILED:
			return HTTP_ERROR_INTERFACE_FAILED;
		case CURLE_OBSOLETE46:
			return HTTP_ERROR_OBSOLETE46;
		case CURLE_TOO_MANY_REDIRECTS :
			return HTTP_ERROR_TOO_MANY_REDIRECTS ;
		case CURLE_UNKNOWN_TELNET_OPTION:
			return HTTP_ERROR_UNKNOWN_TELNET_OPTION;
		case CURLE_TELNET_OPTION_SYNTAX :
			return HTTP_ERROR_TELNET_OPTION_SYNTAX ;
		case CURLE_OBSOLETE50:
			return HTTP_ERROR_OBSOLETE50;
		case CURLE_PEER_FAILED_VERIFICATION:
			return HTTP_ERROR_PEER_FAILED_VERIFICATION;
		case CURLE_GOT_NOTHING:
			return HTTP_ERROR_GOT_NOTHING;
		case CURLE_SSL_ENGINE_NOTFOUND:
			return HTTP_ERROR_SSL_ENGINE_NOTFOUND;
		case CURLE_SSL_ENGINE_SETFAILED:
			return HTTP_ERROR_SSL_ENGINE_SETFAILED;
		case CURLE_SEND_ERROR:
			return HTTP_ERROR_SEND_ERROR;
		case CURLE_RECV_ERROR:
			return HTTP_ERROR_RECV_ERROR;
		case CURLE_OBSOLETE57:
			return HTTP_ERROR_OBSOLETE57;
		case CURLE_SSL_CERTPROBLEM:
			return HTTP_ERROR_SSL_CERTPROBLEM;
		case CURLE_SSL_CIPHER:
			return HTTP_ERROR_SSL_CIPHER;
		case CURLE_SSL_CACERT:
			return HTTP_ERROR_SSL_CACERT;
		case CURLE_BAD_CONTENT_ENCODING:
			return HTTP_ERROR_BAD_CONTENT_ENCODING;
		case CURLE_LDAP_INVALID_URL:
			return HTTP_ERROR_LDAP_INVALID_URL;
		case CURLE_FILESIZE_EXCEEDED:
			return HTTP_ERROR_FILESIZE_EXCEEDED;
		case CURLE_USE_SSL_FAILED:
			return HTTP_ERROR_USE_SSL_FAILED;
		case CURLE_SEND_FAIL_REWIND:
			return HTTP_ERROR_SEND_FAIL_REWIND;
		case CURLE_SSL_ENGINE_INITFAILED:
			return HTTP_ERROR_SSL_ENGINE_INITFAILED;
		case CURLE_LOGIN_DENIED:
			return HTTP_ERROR_LOGIN_DENIED;
		case CURLE_TFTP_NOTFOUND:
			return HTTP_ERROR_TFTP_NOTFOUND;
		case CURLE_TFTP_PERM:
			return HTTP_ERROR_TFTP_PERM;
		case CURLE_REMOTE_DISK_FULL:
			return HTTP_ERROR_REMOTE_DISK_FULL;
		case CURLE_TFTP_ILLEGAL:
			return HTTP_ERROR_TFTP_ILLEGAL;
		case CURLE_TFTP_UNKNOWNID:
			return HTTP_ERROR_TFTP_UNKNOWNID;
		case CURLE_REMOTE_FILE_EXISTS:
			return HTTP_ERROR_REMOTE_FILE_EXISTS;
		case CURLE_TFTP_NOSUCHUSER:
			return HTTP_ERROR_TFTP_NOSUCHUSER;
		case CURLE_CONV_FAILED:
			return HTTP_ERROR_CONV_FAILED;
		case CURLE_CONV_REQD:
			return HTTP_ERROR_CONV_REQD;
		case CURLE_SSL_CACERT_BADFILE:
			return HTTP_ERROR_SSL_CACERT_BADFILE;
		case CURLE_REMOTE_FILE_NOT_FOUND:
			return HTTP_ERROR_REMOTE_FILE_NOT_FOUND;
		case CURLE_SSH:
			return HTTP_ERROR_SSH;
		case CURLE_SSL_SHUTDOWN_FAILED:
			return HTTP_ERROR_SSL_SHUTDOWN_FAILED;
		case CURLE_AGAIN:
			return HTTP_ERROR_AGAIN;
		case CURLE_SSL_CRL_BADFILE:
			return HTTP_ERROR_SSL_CRL_BADFILE;
		case CURLE_SSL_ISSUER_ERROR:
			return HTTP_ERROR_SSL_ISSUER_ERROR;
		case CURLE_FTP_PRET_FAILED:
			return HTTP_ERROR_FTP_PRET_FAILED;
		case CURLE_RTSP_CSEQ_ERROR:
			return HTTP_ERROR_RTSP_CSEQ_ERROR;
		case CURLE_RTSP_SESSION_ERROR:
			return HTTP_ERROR_RTSP_SESSION_ERROR;
		case CURLE_FTP_BAD_FILE_LIST:
			return HTTP_ERROR_FTP_BAD_FILE_LIST;
		case CURLE_CHUNK_FAILED:
			return HTTP_ERROR_CHUNK_FAILED;
		default:
			return HTTP_ERROR_UNKNOWN;
	}
}

CurlSSLVerifier& CurlConnection::GetSSLVerifier()
{
	return m_sslVerifier;
}

std::string CurlConnection::ExtractHostnameFromUrl(const std::string& url)
{
	size_t schemeDelim = url.find("://");
	if (schemeDelim) {
		size_t schemeEnd = schemeDelim + 3; // skip past ://
		size_t hostEnd = url.find("/", schemeEnd);

		if (url.at(schemeEnd) == '[') { // IPv6 literal (RFC 2732)
			// strip the brackets
			schemeEnd++;
			hostEnd = url.rfind(']', hostEnd);
		} else {
			// Note: don't run this code for IPv6 addresses; port is already filtered by ] in that case
			size_t portEnd = url.rfind(':', hostEnd);
			if (portEnd != string::npos && portEnd > schemeEnd) {
				// strip the port number
				hostEnd = portEnd;
			}
		}

		if (hostEnd != string::npos)
			return url.substr(schemeEnd, hostEnd - schemeEnd);
		else
			return url.substr(schemeEnd);
	}

	return "";
}

std::string CurlConnection::GetCurrentHostname()
{
	const char* url;
	CURLcode err = curl_easy_getinfo(m_easyHandle, CURLINFO_EFFECTIVE_URL, &url);
	CheckError(err);

	if (url) {
		string unescapedUrl;

		int outLength = 0;
		char* temp = curl_easy_unescape(m_easyHandle, url, 0, &outLength );
		
		if (temp != NULL) {
			unescapedUrl.assign(temp);
			curl_free(temp);
		}

		return ExtractHostnameFromUrl(unescapedUrl);
	}

	return "";
}
