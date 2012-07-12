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

#ifndef POPERRORS_H_
#define POPERRORS_H_

#include "data/EmailAccount.h"

class PopErrors
{
public:
	/**
	 * If the given account error represents a login error, return true.  Otherwise, false.
	 */
	static bool	IsLoginError(EmailAccount::AccountError err);

	/**
	 * If the given account error represents a network error, return true.  Otherwise, false.
	 */
	static bool	IsNetworkError(EmailAccount::AccountError err);

	/**
	 * If the given account error represents a network retry-able error, return true.  Otherwise, false.
	 */
	static bool	IsRetryNetworkError(EmailAccount::AccountError err);

	/**
	 * If the given account error represents a SSL error, return true.  Otherwise, false.
	 */
	static bool	IsSSLNetworkError(EmailAccount::AccountError err);

	/**
	 * If the given account error represents an account retry-able error, return true.  Otherwise, false.
	 */
	static bool	IsRetryAccountError(EmailAccount::AccountError err);

	/**
	 * If the given account error represents a retry-able error, return true.  Otherwise, false.
	 */
	static bool	IsRetryError(EmailAccount::AccountError err);
};

#endif /* POPERRORS_H_ */
