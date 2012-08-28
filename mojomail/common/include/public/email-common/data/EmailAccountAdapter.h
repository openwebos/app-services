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

#ifndef EMAILACCOUNTADAPTER_H_
#define EMAILACCOUNTADAPTER_H_

#include "core/MojObject.h"
#include "CommonErrors.h"
#include "data/EmailAccount.h"

class EmailAccountAdapter
{
public:
	static const char* const ACCOUNT_ID;

	static const char* const SYNC_WINDOW_DAYS;
	static const char* const SYNC_FREQUENCY_MINS;

	static const char* const ERROR;
	static const char* const ERROR_CODE;
	static const char* const ERROR_TEXT;

	static const char* const RETRY;
	static const char* const RETRY_INTERVAL;
	static const char* const RETRY_COUNT;
	static const char* const RETRY_REASON;

	static const char* const INBOX_FOLDERID;
	static const char* const OUTBOX_FOLDERID;
	static const char* const SENT_FOLDERID;
	static const char* const DRAFTS_FOLDERID;
	static const char* const TRASH_FOLDERID;
	static const char* const ARCHIVE_FOLDERID;
	static const char* const SPAM_FOLDERID;

	static void ParseDatabaseObject(const MojObject& obj, EmailAccount& account);
	static void ParseSpecialFolders(const MojObject& obj, EmailAccount& account);
	static void SerializeSpecialFolders(const EmailAccount& account, MojObject& obj);
	static MailError::ErrorCode GetErrorCode(int in);

	static void ParseErrorInfo(const MojObject& obj, MailError::ErrorInfo& error);
	static void SerializeErrorInfo(const MailError::ErrorInfo& error, MojObject& obj);

	static void ParseAccountRetryStatus(const MojObject& accountObj, EmailAccount& account);
	static void SerializeAccountRetryStatus(const EmailAccount& account, MojObject& accountObj);

	// Appends the special folder ids from the account to a list
	static void AppendSpecialFolderIds(const EmailAccount& account, MojObject::ObjectVec& list);

	// Sets inboxFolderId, draftsFolderId, etc, to null if the existing folder id isn't found in the provided list
	static void ClearMissingFolderIds(EmailAccount& account, const MojObject::ObjectVec& existingIds);

private:
	EmailAccountAdapter();

	static MojObject GetFolderId(const MojObject& obj, const char* prop);
};

#endif /* EMAILACCOUNTADAPTER_H_ */
