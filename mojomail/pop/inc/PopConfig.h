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

#ifndef POPCONFIG_H_
#define POPCONFIG_H_

#include "core/MojCoreDefs.h"
#include <string>

class PopConfig
{
public:
	// the number of email headers to be downloaded and analyzed before persisting pop emails.
	static const int CHECK_REQUESTS_INTERVAL = 5;

	// the number of emails that we want to search back since the first email
	// has the email date beyond the sync back time.  we only use this number
	// when all the previous email are ordered in date.  we look back 50 more
	// emails to make sure that the emails are really ordered by date.
	static const int SYNC_BACK_EMAIL_COUNT = 50;

	// the time difference that is allowed to consider email dates to be in order
	static const int ALLOWABLE_TIME_DISCREPANCY = 15 * 60 * 1000;

	// Time to wait for greeting
	static const int GREETING_RESPONSE_TIMEOUT = 35 * 1000;

	// Time to wait for a response to a user/pass command
	static const int LOGIN_RESPONSE_TIMEOUT = 35 * 1000;

	// for Hotmail, if a message contains large attachment (such as 8MB), it
	// will take a long time to get a response for retrieving message.  So make
	// this read timeout to be 30 seconds.
	static const int READ_TIMEOUT_IN_SECONDS = 30;

	// the maximum emails to be synced down to the device.
	static const int MAX_EMAIL_COUNT_ON_DEVICE = 1000;
};

#endif /* POPCONFIG_H_ */
