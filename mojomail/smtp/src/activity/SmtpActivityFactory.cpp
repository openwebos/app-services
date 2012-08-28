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

#include "activity/SmtpActivityFactory.h"
#include "activity/ActivityBuilder.h"
#include "SmtpCommon.h"

const char* SmtpActivityFactory::OUTBOX_WATCH_ACTIVITY_FMT = "SMTP Watch/accountId=\"%s\"";
const char* SmtpActivityFactory::ACCOUNT_WATCH_ACTIVITY_FMT = "SMTP Account Watch/accountId=\"%s\"";
const char* SmtpActivityFactory::OUTBOX_BUS_METHOD = "palm://com.palm.smtp/syncOutbox";
const char* SmtpActivityFactory::ACCOUNT_UPDATED_BUS_METHOD = "palm://com.palm.smtp/accountUpdated";

void SmtpActivityFactory::BuildSmtpConfigWatch(ActivityBuilder& ab, const MojObject& accountId, MojInt64 rev)
{
	MojErr err;

	MojString name;
	err = name.format(ACCOUNT_WATCH_ACTIVITY_FMT, AsJsonString(accountId).c_str());
	ErrorToException(err);

	// description of watch
	MojString desc;
	err = desc.format("Watches SMTP config on account %s", AsJsonString(accountId).c_str());
	ErrorToException(err);

	// activity to setup watch
	ab.SetName(name);
	ab.SetDescription(desc.data());
	ab.SetPersist(true);
	ab.SetExplicit(true);
	ab.SetRequiresInternet(false); // trigger even if we don't have a network connection
	ab.SetImmediate(true, ActivityBuilder::PRIORITY_LOW);

	// setup trigger
	// NOTE: how to trigger only on SMTP config change?
	MojDbQuery trigger;
	err = trigger.from("com.palm.mail.account:1");
	ErrorToException(err);
	err = trigger.where("accountId", MojDbQuery::OpEq, accountId);
	ErrorToException(err);

	if (rev > 0) {
		trigger.where("_revSmtp", MojDbQuery::OpGreaterThan, rev);
	}
	ab.SetDatabaseWatchTrigger(trigger);

	MojObject params;
	err = params.put("accountId", accountId);
	ErrorToException(err);

	ab.SetCallback(ACCOUNT_UPDATED_BUS_METHOD, params);
	ab.SetMetadata(params);
}

void SmtpActivityFactory::BuildOutboxWatch(ActivityBuilder& ab, const MojObject& accountId, const MojObject& folderId)
{
	ActivityBuilder actBuilder;

	MojString name;

	name.format(OUTBOX_WATCH_ACTIVITY_FMT, AsJsonString(accountId).c_str());

	ab.SetName(name);
	ab.SetDescription("Watches SMTP outbox for new emails");
	ab.SetPersist(true);
	ab.SetExplicit(true);
	ab.SetRequiresInternet(false); // don't trigger until we also have connectivity
	ab.SetImmediate(true, ActivityBuilder::PRIORITY_LOW);

	// Callback
	MojObject callbackParams;
	MojErr err = callbackParams.put("accountId", accountId);
	ErrorToException(err);
	err = callbackParams.put("folderId", folderId);
	ErrorToException(err);

	ab.SetCallback(OUTBOX_BUS_METHOD, callbackParams);
	ab.SetMetadata(callbackParams);
}

