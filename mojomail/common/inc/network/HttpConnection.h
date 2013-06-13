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

#ifndef HTTPCONNECTION_H_
#define HTTPCONNECTION_H_

#include "core/MojCoreDefs.h"
#include <string>
#include "CommonErrors.h"

/**
 * \brief
 * Abstract class which represents an HTTP connection to an EAS server.
 */
class HttpConnection
{
public:
	static const char* const CONTENT_LENGTH;
	static const char* const CONTENT_TYPE;
	static const char* const CONTENT_TYPE_WBXML;
	static const char* const CONTENT_TYPE_RFC822;
	static const char* const EXPECT;
	static const char* const HEADER_POLICY_KEY;
	static const char* const HEADER_PROTOCOL_VERSION;
	static const char* const HEADER_USER_AGENT;

	static const int HTTP_NONE					= 0;
	static const int HTTP_OK 					= 200;
	static const int HTTP_UNAUTHORIZED			= 401;
	static const int HTTP_FORBIDDEN				= 403;
	static const int HTTP_NOT_FOUND				= 404;
	static const int HTTP_TOO_LARGE				= 413;
	static const int HTTP_NEEDS_PROVISIONING 	= 449;
	static const int HTTP_INTERNAL_SERVER_ERROR	= 500;
	static const int HTTP_SERVICE_UNAVAILABLE	= 503;

	typedef enum {
		MethodOptions,
		MethodGet,
		MethodPost
	} ConnectionMethod;

	// Mostly stolen from curl.h
	typedef enum {
		HTTP_ERROR_NONE = 0,
		HTTP_ERROR_UNSUPPORTED_PROTOCOL,    /* 1 */
		HTTP_ERROR_FAILED_INIT,             /* 2 */
		HTTP_ERROR_URL_MALFORMAT,           /* 3 */
		HTTP_ERROR_OBSOLETE4,               /* 4 - NOT USED */
		HTTP_ERROR_COULDNT_RESOLVE_PROXY,   /* 5 */
		HTTP_ERROR_COULDNT_RESOLVE_HOST,    /* 6 */
		HTTP_ERROR_COULDNT_CONNECT,         /* 7 */
		HTTP_ERROR_FTP_WEIRD_SERVER_REPLY,  /* 8 */
		HTTP_ERROR_REMOTE_ACCESS_DENIED,    /* 9 a service was denied by the server
										due to lack of access - when login fails
										this is not returned. */
		HTTP_ERROR_OBSOLETE10,              /* 10 - NOT USED */
		HTTP_ERROR_FTP_WEIRD_PASS_REPLY,    /* 11 */
		HTTP_ERROR_OBSOLETE12,              /* 12 - NOT USED */
		HTTP_ERROR_FTP_WEIRD_PASV_REPLY,    /* 13 */
		HTTP_ERROR_FTP_WEIRD_227_FORMAT,    /* 14 */
		HTTP_ERROR_FTP_CANT_GET_HOST,       /* 15 */
		HTTP_ERROR_OBSOLETE16,              /* 16 - NOT USED */
		HTTP_ERROR_FTP_COULDNT_SET_TYPE,    /* 17 */
		HTTP_ERROR_PARTIAL_FILE,            /* 18 */
		HTTP_ERROR_FTP_COULDNT_RETR_FILE,   /* 19 */
		HTTP_ERROR_OBSOLETE20,              /* 20 - NOT USED */
		HTTP_ERROR_QUOTE_ERROR,             /* 21 - quote command failure */
		HTTP_ERROR_HTTP_RETURNED_ERROR,     /* 22 */
		HTTP_ERROR_WRITE_ERROR,             /* 23 */
		HTTP_ERROR_OBSOLETE24,              /* 24 - NOT USED */
		HTTP_ERROR_UPLOAD_FAILED,           /* 25 - failed upload "command" */
		HTTP_ERROR_READ_ERROR,              /* 26 - couldn't open/read from file */
		HTTP_ERROR_OUT_OF_MEMORY,           /* 27 */
		/* Note: HTTP_ERROR_OUT_OF_MEMORY may sometimes indicate a conversion error
			   instead of a memory allocation error if CURL_DOES_CONVERSIONS
			   is defined
		*/
		HTTP_ERROR_OPERATION_TIMEDOUT,      /* 28 - the timeout time was reached */
		HTTP_ERROR_OBSOLETE29,              /* 29 - NOT USED */
		HTTP_ERROR_FTP_PORT_FAILED,         /* 30 - FTP PORT operation failed */
		HTTP_ERROR_FTP_COULDNT_USE_REST,    /* 31 - the REST command failed */
		HTTP_ERROR_OBSOLETE32,              /* 32 - NOT USED */
		HTTP_ERROR_RANGE_ERROR,             /* 33 - RANGE "command" didn't work */
		HTTP_ERROR_HTTP_POST_ERROR,         /* 34 */
		HTTP_ERROR_SSL_CONNECT_ERROR,       /* 35 - wrong when connecting with SSL */
		HTTP_ERROR_BAD_DOWNLOAD_RESUME,     /* 36 - couldn't resume download */
		HTTP_ERROR_FILE_COULDNT_READ_FILE,  /* 37 */
		HTTP_ERROR_LDAP_CANNOT_BIND,        /* 38 */
		HTTP_ERROR_LDAP_SEARCH_FAILED,      /* 39 */
		HTTP_ERROR_OBSOLETE40,              /* 40 - NOT USED */
		HTTP_ERROR_FUNCTION_NOT_FOUND,      /* 41 */
		HTTP_ERROR_ABORTED_BY_CALLBACK,     /* 42 */
		HTTP_ERROR_BAD_FUNCTION_ARGUMENT,   /* 43 */
		HTTP_ERROR_OBSOLETE44,              /* 44 - NOT USED */
		HTTP_ERROR_INTERFACE_FAILED,        /* 45 - CURLOPT_INTERFACE failed */
		HTTP_ERROR_OBSOLETE46,              /* 46 - NOT USED */
		HTTP_ERROR_TOO_MANY_REDIRECTS ,     /* 47 - catch endless re-direct loops */
		HTTP_ERROR_UNKNOWN_TELNET_OPTION,   /* 48 - User specified an unknown option */
		HTTP_ERROR_TELNET_OPTION_SYNTAX ,   /* 49 - Malformed telnet option */
		HTTP_ERROR_OBSOLETE50,              /* 50 - NOT USED */
		HTTP_ERROR_PEER_FAILED_VERIFICATION, /* 51 - peer's certificate or fingerprint
										 wasn't verified fine */
		HTTP_ERROR_GOT_NOTHING,             /* 52 - when this is a specific error */
		HTTP_ERROR_SSL_ENGINE_NOTFOUND,     /* 53 - SSL crypto engine not found */
		HTTP_ERROR_SSL_ENGINE_SETFAILED,    /* 54 - can not set SSL crypto engine as
										default */
		HTTP_ERROR_SEND_ERROR,              /* 55 - failed sending network data */
		HTTP_ERROR_RECV_ERROR,              /* 56 - failure in receiving network data */
		HTTP_ERROR_OBSOLETE57,              /* 57 - NOT IN USE */
		HTTP_ERROR_SSL_CERTPROBLEM,         /* 58 - problem with the local certificate */
		HTTP_ERROR_SSL_CIPHER,              /* 59 - couldn't use specified cipher */
		HTTP_ERROR_SSL_CACERT,              /* 60 - problem with the CA cert (path?) */
		HTTP_ERROR_BAD_CONTENT_ENCODING,    /* 61 - Unrecognized transfer encoding */
		HTTP_ERROR_LDAP_INVALID_URL,        /* 62 - Invalid LDAP URL */
		HTTP_ERROR_FILESIZE_EXCEEDED,       /* 63 - Maximum file size exceeded */
		HTTP_ERROR_USE_SSL_FAILED,          /* 64 - Requested FTP SSL level failed */
		HTTP_ERROR_SEND_FAIL_REWIND,        /* 65 - Sending the data requires a rewind
										that failed */
		HTTP_ERROR_SSL_ENGINE_INITFAILED,   /* 66 - failed to initialise ENGINE */
		HTTP_ERROR_LOGIN_DENIED,            /* 67 - user, password or similar was not
										accepted and we failed to login */
		HTTP_ERROR_TFTP_NOTFOUND,           /* 68 - file not found on server */
		HTTP_ERROR_TFTP_PERM,               /* 69 - permission problem on server */
		HTTP_ERROR_REMOTE_DISK_FULL,        /* 70 - out of disk space on server */
		HTTP_ERROR_TFTP_ILLEGAL,            /* 71 - Illegal TFTP operation */
		HTTP_ERROR_TFTP_UNKNOWNID,          /* 72 - Unknown transfer ID */
		HTTP_ERROR_REMOTE_FILE_EXISTS,      /* 73 - File already exists */
		HTTP_ERROR_TFTP_NOSUCHUSER,         /* 74 - No such user */
		HTTP_ERROR_CONV_FAILED,             /* 75 - conversion failed */
		HTTP_ERROR_CONV_REQD,               /* 76 - caller must register conversion
										callbacks using curl_easy_setopt options
										CURLOPT_CONV_FROM_NETWORK_FUNCTION,
										CURLOPT_CONV_TO_NETWORK_FUNCTION, and
										CURLOPT_CONV_FROM_UTF8_FUNCTION */
		HTTP_ERROR_SSL_CACERT_BADFILE,      /* 77 - could not load CACERT file, missing
										or wrong format */
		HTTP_ERROR_REMOTE_FILE_NOT_FOUND,   /* 78 - remote file not found */
		HTTP_ERROR_SSH,                     /* 79 - error from the SSH layer, somewhat
										generic so the error message will be of
										interest when this has happened */

		HTTP_ERROR_SSL_SHUTDOWN_FAILED,     /* 80 - Failed to shut down the SSL
										connection */
		HTTP_ERROR_AGAIN,                   /* 81 - socket is not ready for send/recv,
										wait till it's ready and try again (Added
										in 7.18.2) */
		HTTP_ERROR_SSL_CRL_BADFILE,         /* 82 - could not load CRL file, missing or
										wrong format (Added in 7.19.0) */
		HTTP_ERROR_SSL_ISSUER_ERROR,        /* 83 - Issuer check failed.  (Added in
										7.19.0) */
		HTTP_ERROR_FTP_PRET_FAILED,         /* 84 - a PRET command failed */
		HTTP_ERROR_RTSP_CSEQ_ERROR,         /* 85 - mismatch of RTSP CSeq numbers */
		HTTP_ERROR_RTSP_SESSION_ERROR,      /* 86 - mismatch of RTSP Session Identifiers */
		HTTP_ERROR_FTP_BAD_FILE_LIST,       /* 87 - unable to parse FTP file list */
		HTTP_ERROR_CHUNK_FAILED,            /* 88 - chunk callback reported error */

		// Non-curl errors
		HTTP_ERROR_SSL_CERTIFICATE_EXPIRED = 1000,     /* 1000 - SSL certificate expired */
		HTTP_ERROR_SSL_CERTIFICATE_NOT_TRUSTED = 1001, /* 1001 - SSL certificate not trusted */
		HTTP_ERROR_SSL_CERTIFICATE_INVALID = 1002,     /* 1002 - SSL certificate error */
		HTTP_ERROR_SSL_HOST_NAME_MISMATCHED = 1003,    /* 1003 - mismatched hostname */

		HTTP_ERROR_UNKNOWN /* never use! */
	} ConnectionError;

	HttpConnection();
	virtual ~HttpConnection();

	/**
	 * Sets the HTTP method (POST, GET, or OPTIONS)
	 */
	virtual void SetMethod(ConnectionMethod method) = 0;

	/**
	 * Adds an HTTP header to be sent upon connection.
	 * (Example: "Content-Type: text/html")
	 */
	virtual void SetHeader(const std::string& header) = 0;

	/**
	 * Sets the HTTP basic authorization header based on the credentials.
	 */
	virtual void SetCredentials(const std::string& username, const std::string& password) = 0;

	/**
	 * The URL to connect to.
	 */
	virtual void SetUrl(const std::string& url) = 0;

	/**
	 * @return the URL to connect to.
	 */
	virtual std::string GetUrl() const = 0;

	/**
	 * @param number of seconds to for the read/write timeout
	 */
	virtual void SetTimeout(long seconds) = 0;

	/**
	 * The data to be sent via HTTP POST.  SetMethod does not need to be called, POST is implied.
	 * The data will NOT be copied and must exist during the lifetime of this connection.
	 */
	virtual void SetPostData(void* data, size_t size) = 0;
	
	/**
	 * Use streaming mode for HTTP POST.  The command's ReadCallback function will be called to
	 * get data for writing.
	 * 
	 * @param  total size of POST data to written
	 */
	virtual void SetStreamingPost(size_t size) = 0;

	/**
	 * Send an empty post (i.e. no data with the POST request);
	 */
	virtual void SetEmptyPost() = 0;

	/**
	 * Do not allow connection re-use.
	 */

	virtual void DisableConnectionReuse() = 0;

	/**
	 * Connects to the server.  Should be called only after all headers/data have been set.
	 */
	virtual void Connect() = 0;

	/**
	 * @return the HTTP response code
	 * @see http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
	 */
	virtual long GetResponseCode() = 0;

	/**
	 * @return a URL encoded string
	 */
	virtual std::string UrlEncode(const std::string& str) = 0;

	/**
	 * Pauses the HTTP connection.  Resume() must be called to receive more data.
	 */
	virtual void Pause() = 0;

	/**
	 * Resumes the HTTP connection.
	 */
	virtual void Resume() = 0;

	virtual bool IsPaused() const = 0;

	/**
	 * @return true if the connection is complete (will not send/receive any more data)
	 */
	virtual bool IsComplete() { return m_complete; };

	/**
	 * Returns the maximum numer of bytes that can be received from the server at once.
	 */
	virtual size_t MaxWriteBufferSize() = 0;

	/**
	 * Gets the MailError::ErrorCode corresponding to a ConnectionError
	 */
	static MailError::ErrorCode GetMailErrorCode(ConnectionError err);

	/**
	 * Gets the MailError::ErrorCode corresponding to a HTTP response code
	 */
	static MailError::ErrorCode GetHttpMailErrorCode(long httpResponseCode);

	/**
	 * Gets the MailError::ErrorCode from either the connection error or HTTP response code
	 */
	static MailError::ErrorCode GetMailErrorCode(ConnectionError conectionErr, long httpResponseCode);

protected:
	bool m_complete;
};

#endif /* HTTPCONNECTION_H_ */
