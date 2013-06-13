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

#ifndef MAILEXCEPTION_H_
#define MAILEXCEPTION_H_

#include <exception>
#include <string>
#include "CommonErrors.h"

class MailException : public std::exception
{
public:
	MailException(const char* file, int line);
	MailException(const char* msg, const char* file, int line);
	virtual ~MailException() throw() { };
	virtual const char* what() const throw();

	// Get mail error info
	virtual MailError::ErrorInfo GetErrorInfo() const;

protected:
	void SetMessage(const char* msg);

	std::string m_msg;
	std::string m_file;
	int m_line;

};

class MailNetworkTimeoutException : public MailException
{
public:
	MailNetworkTimeoutException(const char* file, int line) : MailException(file, line) {}
	MailNetworkTimeoutException(const char* msg, const char* file, int line) : MailException(msg,file,line) {}
	virtual ~MailNetworkTimeoutException() throw() { }

	virtual MailError::ErrorInfo GetErrorInfo() const
	{
		return MailError::ErrorInfo(MailError::CONNECTION_TIMED_OUT);
	}
};

class MailNetworkDisconnectionException : public MailException
{
public:
	MailNetworkDisconnectionException(const char* file, int line) : MailException(file, line) {}
	MailNetworkDisconnectionException(const char* msg, const char* file, int line) : MailException(msg,file,line) {}
	virtual ~MailNetworkDisconnectionException() throw() { }

	virtual MailError::ErrorInfo GetErrorInfo() const
	{
		return MailError::ErrorInfo(MailError::CONNECTION_FAILED);
	}
};

class MailFileCacheNotCreatedException : public MailException
{
public:
	MailFileCacheNotCreatedException(const char* file, int line) : MailException(file, line) {}
	MailFileCacheNotCreatedException(const char* msg, const char* file, int line) : MailException(msg,file,line) {}
	virtual ~MailFileCacheNotCreatedException() throw() { }

	virtual MailError::ErrorInfo GetErrorInfo() const
	{
		return MailError::ErrorInfo(MailError::EMAIL_SIZE_EXCEEDED);
	}
};

#endif /*MAILEXCEPTION_H_*/
