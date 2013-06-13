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

#include "data/EmailAccountAdapter.h"
#include "data/EmailAccount.h"
#include "CommonPrivate.h"

const char* const EmailAccountAdapter::ACCOUNT_ID			= "accountId";

const char* const EmailAccountAdapter::SYNC_WINDOW_DAYS		= "syncWindowDays";
const char* const EmailAccountAdapter::SYNC_FREQUENCY_MINS	= "syncFrequencyMins";

const char* const EmailAccountAdapter::ERROR				= "error";
const char* const EmailAccountAdapter::ERROR_CODE			= "errorCode";
const char* const EmailAccountAdapter::ERROR_TEXT			= "errorText";

const char* const EmailAccountAdapter::RETRY				= "retry";
const char* const EmailAccountAdapter::RETRY_INTERVAL		= "interval";
const char* const EmailAccountAdapter::RETRY_COUNT			= "count";
const char* const EmailAccountAdapter::RETRY_REASON			= "reason";

const char* const EmailAccountAdapter::INBOX_FOLDERID		= "inboxFolderId";
const char* const EmailAccountAdapter::OUTBOX_FOLDERID		= "outboxFolderId";
const char* const EmailAccountAdapter::SENT_FOLDERID		= "sentFolderId";
const char* const EmailAccountAdapter::DRAFTS_FOLDERID		= "draftsFolderId";
const char* const EmailAccountAdapter::TRASH_FOLDERID		= "trashFolderId";
const char* const EmailAccountAdapter::ARCHIVE_FOLDERID		= "archiveFolderId";
const char* const EmailAccountAdapter::SPAM_FOLDERID		= "spamFolderId";

MojObject EmailAccountAdapter::GetFolderId(const MojObject& obj, const char* prop)
{
	MojObject folderId(MojObject::Undefined);
	if(obj.contains(prop)) {
		MojErr err = obj.getRequired(prop, folderId);
		ErrorToException(err);
	}
	return folderId;
}

void EmailAccountAdapter::ParseDatabaseObject(const MojObject& obj, EmailAccount& account)
{
	MojErr err;

	ParseSpecialFolders(obj, account);

	MojObject accountId;
	err = obj.getRequired(ACCOUNT_ID, accountId);
	ErrorToException(err);
	account.SetAccountId(accountId);

	int syncWindowDays = 0;
	err = obj.getRequired(SYNC_WINDOW_DAYS, syncWindowDays);
	ErrorToException(err);
	account.SetSyncWindowDays(syncWindowDays);

	int syncFrequencyMins = 0;
	err = obj.getRequired(SYNC_FREQUENCY_MINS, syncFrequencyMins);
	ErrorToException(err);
	account.SetSyncFrequencyMins(syncFrequencyMins);

	// Retrieve the optional error status
	MojObject errorObj;
	if(obj.get(ERROR, errorObj) && !errorObj.null())
	{
		MailError::ErrorInfo accountError;
		ParseErrorInfo(errorObj, accountError);

		account.SetError(accountError);
	}

	ParseAccountRetryStatus(obj, account);
}

void EmailAccountAdapter::ParseSpecialFolders(const MojObject& obj, EmailAccount& account)
{
	// Update folder ids on EmailAccount. Non-existent properties will be set to undefined.
	account.SetInboxFolderId( GetFolderId(obj, INBOX_FOLDERID) );
	account.SetArchiveFolderId( GetFolderId(obj, ARCHIVE_FOLDERID) );
	account.SetDraftsFolderId( GetFolderId(obj, DRAFTS_FOLDERID) );
	account.SetSentFolderId( GetFolderId(obj, SENT_FOLDERID) );
	account.SetOutboxFolderId( GetFolderId(obj, OUTBOX_FOLDERID) );
	account.SetSpamFolderId( GetFolderId(obj, SPAM_FOLDERID) );
	account.SetTrashFolderId( GetFolderId(obj, TRASH_FOLDERID) );
}

void EmailAccountAdapter::SerializeSpecialFolders(const EmailAccount& account, MojObject& obj)
{
	MojErr err;

	err = obj.put(INBOX_FOLDERID, account.GetInboxFolderId());
	ErrorToException(err);

	err = obj.put(ARCHIVE_FOLDERID, account.GetArchiveFolderId());
	ErrorToException(err);

	err = obj.put(DRAFTS_FOLDERID, account.GetDraftsFolderId());
	ErrorToException(err);

	err = obj.put(SENT_FOLDERID, account.GetSentFolderId());
	ErrorToException(err);

	err = obj.put(OUTBOX_FOLDERID, account.GetOutboxFolderId());
	ErrorToException(err);

	err = obj.put(SPAM_FOLDERID, account.GetSpamFolderId());
	ErrorToException(err);

	err = obj.put(TRASH_FOLDERID, account.GetTrashFolderId());
	ErrorToException(err);
}

MailError::ErrorCode EmailAccountAdapter::GetErrorCode(int in)
{
	return (MailError::ErrorCode)in;
}

void EmailAccountAdapter::SerializeErrorInfo(const MailError::ErrorInfo &error, MojObject& obj)
{
	MojErr err;

	err = obj.put(ERROR_CODE, error.errorCode);
	ErrorToException(err);

	err = obj.putString(ERROR_TEXT, error.errorText.c_str());
	ErrorToException(err);
}

void EmailAccountAdapter::ParseErrorInfo(const MojObject& obj, MailError::ErrorInfo& error)
{
	MojErr err;

	if(obj.null()) {
		error.errorCode = MailError::NONE;
		return;
	}

	MojInt64 errorCode;
	if(obj.get(ERROR_CODE, errorCode)) {
		error.errorCode = GetErrorCode(errorCode);
	}

	bool hasErrorText = false;
	MojString errorText;
	err = obj.get(ERROR_TEXT, errorText, hasErrorText);
	ErrorToException(err);

	if(hasErrorText) {
		error.errorText = errorText.data();
	}
}

void EmailAccountAdapter::ParseAccountRetryStatus(const MojObject& emailAccountObj, EmailAccount& emailAccount)
{
	MojErr err;

	EmailAccount::RetryStatus retryStatus;
	MojObject retryObj;

	if(emailAccountObj.get(RETRY, retryObj)) {
		if(!retryObj.null()) {
			int interval;
			err = retryObj.getRequired(RETRY_INTERVAL, interval);
			ErrorToException(err);
			retryStatus.SetInterval(interval);

			bool hasRetryCount = false;
			int retryCount = 0;
			err = retryObj.get(RETRY_COUNT, retryCount, hasRetryCount);
			ErrorToException(err);
			if(hasRetryCount && retryCount >= 0) {
				retryStatus.SetCount(retryCount);
			}

			bool hasReason = false;
			MojString reason;
			err = retryObj.get(RETRY_REASON, reason, hasReason);
			ErrorToException(err);
			if(hasReason) {
				retryStatus.SetReason(reason.data());
			}

			MojObject errorObj;
			if(retryObj.get(ERROR, errorObj) && !errorObj.null()) {
				MailError::ErrorInfo errorInfo;
				ParseErrorInfo(errorObj, errorInfo);

				retryStatus.SetError(errorInfo);
			}
		}
	}

	emailAccount.SetRetry(retryStatus);
}

void EmailAccountAdapter::SerializeAccountRetryStatus(const EmailAccount& emailAccount, MojObject& accountObj)
{
	MojErr err;
	const EmailAccount::RetryStatus& retryStatus = emailAccount.GetRetry();

	if(retryStatus.GetInterval() > 0) {
		MojObject retryObj;
		err = retryObj.put(RETRY_INTERVAL, retryStatus.GetInterval());
		ErrorToException(err);

		err = retryObj.put(RETRY_COUNT, retryStatus.GetCount());
		ErrorToException(err);

		if(!retryStatus.GetReason().empty()) {
			err = accountObj.putString(RETRY_REASON, retryStatus.GetReason().c_str());
			ErrorToException(err);
		}

		if(retryStatus.GetError().errorCode != MailError::NONE) {
			MojObject errorObj;

			SerializeErrorInfo(retryStatus.GetError(), errorObj);

			err = retryObj.put(ERROR, errorObj);
			ErrorToException(err);
		}
		
		err = accountObj.put(RETRY, retryObj);
		ErrorToException(err);
	} else {
		// no retry
		MojObject retryObj(MojObject::TypeNull);

		err = accountObj.put(RETRY, retryObj);
		ErrorToException(err);
	}
}

inline void AppendIfValid(MojObject::ObjectVec& list, const MojObject& id)
{
	if(IsValidId(id)) {
		MojErr err = list.push(id);
		ErrorToException(err);
	}
}

void EmailAccountAdapter::AppendSpecialFolderIds(const EmailAccount& account, MojObject::ObjectVec& list)
{
	AppendIfValid(list, account.GetInboxFolderId());
	AppendIfValid(list, account.GetDraftsFolderId());
	AppendIfValid(list, account.GetSentFolderId());
	AppendIfValid(list, account.GetOutboxFolderId());
	AppendIfValid(list, account.GetTrashFolderId());
}

void EmailAccountAdapter::ClearMissingFolderIds(EmailAccount& account, const MojObject::ObjectVec& existingIds)
{
	if(existingIds.find(account.GetInboxFolderId()) == MojInvalidIndex) {
		account.SetInboxFolderId(MojObject::Null);
	}

	if(existingIds.find(account.GetDraftsFolderId()) == MojInvalidIndex) {
		account.SetDraftsFolderId(MojObject::Null);
	}

	if(existingIds.find(account.GetSentFolderId()) == MojInvalidIndex) {
		account.SetSentFolderId(MojObject::Null);
	}

	if(existingIds.find(account.GetOutboxFolderId()) == MojInvalidIndex) {
		account.SetOutboxFolderId(MojObject::Null);
	}

	if(existingIds.find(account.GetTrashFolderId()) == MojInvalidIndex) {
		account.SetTrashFolderId(MojObject::Null);
	}
}
