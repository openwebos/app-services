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

#ifndef COMMONERRORS_H_
#define COMMONERRORS_H_

#include <string>

/**
 * These match the UI error codes found at:
 * http://subversion.palm.com/main/nova/palm/luna/apps/email/trunk/app/models/EmailAccountValidation.js
 */
namespace MailError {
	typedef enum {
			NONE = 0,
			VALIDATED = 0,

			// Account Error Codes
			BAD_USERNAME_OR_PASSWORD = 1000,      	// invalid user name or password
			MISSING_CREDENTIALS = 1010,				// no password saved (e.g. after a restore)
			ACCOUNT_LOCKED = 1100,        		// user account is locked
			ACCOUNT_UNAVAILABLE = 1110,       	// user account is temporarily unavailable
			ACCOUNT_WEB_LOGIN_REQUIRED = 1120,  // web login is required to unlocked user account
			ACCOUNT_UNKNOWN_AUTH_ERROR = 1299,  // generic unknown authentication error
			ACCOUNT_NEEDS_PROVISIONING = 1310,	// account needs to be provisioned
			LOGIN_TIMEOUT = 1320,		// login timed out; possibly due to a bad password
			// Reserved EAS account specific errors: 1500-1649
			// Reserved IMAP account specific errors: 1650-1799
			IMAP_NOT_ENABLED = 1650,			// IMAP needs to be enabled from the website
			// Reserved POP account specific errors: 1800-1899
			POP_NOT_ENABLED = 1800,				// POP needs to be enabled from the website
			// Reserved SMTP account specific errors: 1900-1999
			AUTHENTICATION_REQUIRED = 1900,   	// SMTP specific error that indicates device needs to authenticate before sending email

			// Security Policy Error Codes
			SECURITY_POLICY_NOT_SUPPORTED = 2000, // Security policy is not supported by device
			SECURITY_POLICY_CHECK_NEEDED = 2500,
			//Reserved EAS security policy specific errors: 2500-2649

			// Connection Error Codes
			HOST_NOT_FOUND = 3000,        		// host name is not found
			CONNECTION_TIMED_OUT = 3010,      	// time-out to get a connection
			CONNECTION_FAILED = 3099,     		// generic connection failure
			NO_NETWORK = 3200,            		// no connectivity
			// Reserved EAS connection specific errors: 3500-3649
			// Reserved IMAP connection specific errors: 3650-3799
			// Reserved POP connection specific erros: 3800-3899
			// Reserved SMTP connection specific errors: 3900-3999

			// SSL Error Codes
			SSL_CERTIFICATE_EXPIRED = 4000,	   	// SSL certificate has expired or is not yet valid
			SSL_CERTIFICATE_NOT_TRUSTED = 4010, // SSL certificate is not trusted
			SSL_CERTIFICATE_INVALID = 4020,	   	// SSL certificate is not valid (or other unknown error)
			SSL_HOST_NAME_MISMATCHED = 4100,	// Host name in certificate doesn't match mail server
			// Reserved EAS specific errors: 4500-4649
			// Reserved IMAP specific errors: 4650-4799
			// Reserved POP specific erros: 4800-4899
			// Reserved SMTP specific errors: 4900-4999
			SMTP_UNKNOWN_ERROR = 4900,			// SMTP server responded with unexpected error
			SMTP_SHUTTING_DOWN = 4901,			// SMTP server is disconnecting in the middle of a transaction
			SMTP_OUTBOX_UNAVAILABLE = 4910,		// SMTP not able to get outbox for the associated account.

			// Server Request Error Codes
			MAILBOX_FULL = 5000,          		// mail box is full
			FOLDER_NOT_FOUND = 5100,      		// folder does not exist in server any more
			FOLDER_CORRUPTED_ON_SERVER = 5110,  // folder is corrupted in server
			EMAIL_NOT_FOUND = 5200,       		// email is not found in server any more
			ATTACHMENT_TOO_LARGE = 5210,		// HTTP 413 error from the server
			EMAIL_PARSER_ERROR = 5220,			// Failed to parse an email
			SERVER_ERROR = 5300,				// Error on server
			// Reserved EAS specific errors for server request: 5500-5649
			// (The following EAS specific errors are retrieved from p348-p349 in EAS.pdf)
			EAS_PROTOCOL_VERSION_MISMATCHED = 5502, // EAS protocol version mismatched
			EAS_INVALID_SYNC_KEY = 5503,        // invalid synchronization key
			EAS_PROTOCOL_ERROR = 5504,          // protocol error
			EAS_SERVER_ERROR = 5505,          	// server error
			EAS_CLIENT_SERVER_CONVERSION_ERROR = 5506, // error in client/server conversion
			EAS_OBJECT_MISMATCHED = 5507,       // conflict matching the client and server object
			EAS_OBJECT_NOT_FOUND = 5508,        // object not found
			// The EAS error code for user account may be out of disk space should be mapped to 'MAILBOX_FULL' value
			EAS_SET_NOTIFICATION_GUID_ERROR = 5510, // error occurred while setting notificaion GUID
			EAS_NEED_PROVISION_FOR_NOTIFICATION = 5511, // device has not been provisioned for notifications
			EAS_FOLDER_HIERARCHY_CHANGED = 5512, // foder Hierarchy has changed
			EAS_EMPTY_SYNC_REQUEST_ERROR = 5513, // server is unable to process empty Sync request
			EAS_INVALID_WAIT_INTERVAL = 5514,   // wait-interval specified by device is outside the range set by the server administrator
			EAS_SYNC_FOLDERS_LIMIT_EXCEEDED = 5515, // exceeds the maximum number of folder that can be synchronized
			EAS_INDETERMINATE_STATE = 5516,     // indeterminate state
			// Reserved IMAP specific errors for server request: 5650-5799
			IMAP_SERVER_ALERT = 5650,			// alert from server (text should be displayed to user)
			IMAP_PROTOCOL_ERROR = 5660,			// server is misbehaving
			// Reserved POP specific erros for server request: 5800-5899
			// Reserved SMTP specific errors for server request: 5900-5999

			// Send Mail Error Codes
			EMAIL_SIZE_EXCEEDED = 6000,			// cannot send an email whose size exceeds the server limit
			BAD_RECIPIENTS = 6010,				// one or more email recipients are invalid

			// Configuration errors, usually indicating errors in manual config
			BAD_PROTOCOL_CONFIG = 9800,			// generic error; specified server doesn't seem to speak IMAP/POP/EAS/SMTP
			EAS_CONFIG_BAD_URL	= 9810,			// URL doesn't appear to be a valid EAS server URL
			CONFIG_NO_SSL		= 9820,			// Requested encryption is not supported
			CONFIG_NEEDS_SSL	= 9821,			// Must enable encryption to continue
			SMTP_CONFIG_UNAVAILABLE = 9830,		// SMTP config not available.

			// Internal Error Code
			INTERNAL_ERROR = 10000,				// generic internal error
			INTERNAL_ACCOUNT_MISCONFIGURED = 10001,		// some consistency error trying to use account, possibly transitory
	} ErrorCode;

	struct ErrorInfo
	{
		ErrorInfo(ErrorCode errorCode = NONE, const std::string& errorText = "")
		: errorCode(errorCode), errorText(errorText) {}
		virtual ~ErrorInfo() {}

		ErrorCode		errorCode;
		std::string 	errorText;
	};

	// Get the accounts app error code for this MailError
	std::string GetAccountErrorCode(ErrorCode mailErrorCode);

	// Returns true if this is a connection error
	bool IsConnectionError(ErrorCode mailErrorCode);

	// Returns true if this is a SSL error
	bool IsSSLError(ErrorCode mailErrorCode);
}


#endif /* COMMONERRORS_H_ */
