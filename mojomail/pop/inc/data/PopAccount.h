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

#ifndef POPACCOUNT_H_
#define POPACCOUNT_H_

#include <string>
#include "core/MojObject.h"
#include "data/EmailAccount.h"
#include "CommonErrors.h"

/**
 * A POP email account object that holds extra properties that are specific to
 * POP account.
 */
class PopAccount : public EmailAccount
{
public:
	static const char* const SOCKET_ENCRYPTION_NONE;
	static const char* const SOCKET_ENCRYPTION_SSL;
	static const char* const SOCKET_ENCRYPTION_TLS;
	static const char* const SOCKET_ENCRYPTION_TLS_IF_AVAILABLE;
	static const int 		 DEFAULT_SYNC_WINDOW;
	static const int 		 DEFAULT_SYNC_FREQUENCY_MINS;

	PopAccount();
	virtual ~PopAccount();
	
	// Setters
	void SetId(const MojObject& id)		 					{ m_id = id; }
	void SetHostName(const std::string& hostname)			{ m_hostname = hostname; }
	void SetPort(int port)									{ m_port = port; }
	void SetEncryption(const std::string& encryption)		{ m_encryption = encryption; }
	void SetUsername(const std::string& username) 			{ m_username = username; }
	void SetPassword(const std::string& password)			{ m_password = password; }
	bool HasPassword()										{ return !m_password.empty();}
	void SetInitialSync(bool sync)						    { m_initialSync = sync; }
	void SetDeleteFromServer(bool del)						{ m_deleteFromServer = del; }
	void SetDeleteOnDevice(bool del)						{ m_deleteOnDevice = del; }
	
	// Getters
	MojObject				GetId() const					{ return m_id; }
	const std::string&		GetHostName() const				{ return m_hostname; }
	const int				GetPort() const					{ return m_port; }
	const std::string&		GetEncryption() const			{ return m_encryption; }
	const std::string&		GetUsername() const				{ return m_username; }
	const std::string&		GetPassword() const				{ return m_password; }
	const bool				IsInitialSync() const			{ return m_initialSync; }
	const bool				IsDeleteFromServer() const		{ return m_deleteFromServer; }
	const bool				IsDeleteOnDevice() const		{ return m_deleteOnDevice; }
	bool					IsDeleted()						{ return false; } //TODO: need to use the '_del' flags
	const bool				IsError() const					{ return m_error.errorCode != MailError::VALIDATED; }

private:
	// properties that are specific to POP transport
	MojObject	m_id;		// com.palm.account:1 database ID
	std::string m_hostname;
	int			m_port;
	std::string m_encryption;
	std::string m_username;
	std::string m_password;
	bool		m_initialSync;
	bool		m_deleteFromServer;	// a flag indicates whether during inbox sync we should delete emails, which are deleted on device, from server.
	bool		m_deleteOnDevice;	// a flag indicates whether during inbox sync we should delete emails, which are deleted on server, from device.
};

#endif /* POPACCOUNT_H_ */
