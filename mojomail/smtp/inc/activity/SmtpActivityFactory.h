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

#ifndef SMTPACTIVITYFACTORY_H_
#define SMTPACTIVITYFACTORY_H_

#include "core/MojObject.h"

class ActivityBuilder;

class SmtpActivityFactory
{
public:
	SmtpActivityFactory() {}
	virtual ~SmtpActivityFactory() {}

	static const char* OUTBOX_WATCH_ACTIVITY_FMT;
	static const char* ACCOUNT_WATCH_ACTIVITY_FMT;
	static const char* OUTBOX_BUS_METHOD;
	static const char* ACCOUNT_UPDATED_BUS_METHOD;

	void BuildSmtpConfigWatch(ActivityBuilder& ab, const MojObject& accountId, MojInt64 rev);
	void BuildOutboxWatch(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId);
};

#endif /* SMTPACTIVITYFACTORY_H_ */
