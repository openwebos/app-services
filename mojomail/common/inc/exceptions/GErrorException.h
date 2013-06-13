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

#ifndef GERROREXCEPTION_H_
#define GERROREXCEPTION_H_

#include "exceptions/MailException.h"
#include "CommonErrors.h"
#include <glib.h>
#include "palmsockerror.h"

class GErrorException : public MailException
{
public:
	GErrorException(const GError* error, const char* filename, int line);
	virtual ~GErrorException() throw() {}
	
	static void CheckError(GError* err, const char* filename, int line);
	static GErrorException ThrowAndFreeError(GError* err, const char* filename, int line);

	MailError::ErrorInfo GetErrorInfo() const;

protected:
	GQuark	m_domain;
	gint	m_code;
};

class PslErrorException : public MailException
{
public:
	PslErrorException(PslError pslError, const char* filename, int line);
	virtual ~PslErrorException() throw() {}

	PslError GetPslError() const { return m_pslError; }

	void SetSSLVerifyResult(int sslVerifyResult);

	virtual MailError::ErrorInfo GetErrorInfo() const;

	static MailError::ErrorInfo GetSSLVerifyErrorInfo(int verifyResult);

protected:
	PslError		m_pslError;
	int				m_sslVerifyResult;
};

#endif /*GERROREXCEPTION_H_*/
