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

#ifndef EMAILACCOUNT_H_
#define EMAILACCOUNT_H_

#include "core/MojObject.h"
#include "CommonErrors.h"
#include <string>

class EmailAccount
{
public:
	typedef MailError::ErrorInfo AccountError;

	class RetryStatus
	{
	public:
		RetryStatus() : m_interval(0), m_count(0) {}
		virtual ~RetryStatus() {}

		int GetInterval() const { return m_interval; }
		void SetInterval(int interval) { m_interval = interval; }

		int GetCount() const { return m_count; }
		void SetCount(int count) { m_count = count; }

		const std::string& GetReason() const { return m_reason; }
		void SetReason(const std::string& reason) { m_reason = reason; }

		const MailError::ErrorInfo& GetError() const { return m_error; }
		void SetError(const MailError::ErrorInfo& error) { m_error = error; }

	private:
		int						m_interval;
		int						m_count;
		std::string				m_reason;
		MailError::ErrorInfo	m_error;
	};

	EmailAccount();
	virtual ~EmailAccount();

	static const int SYNC_FREQUENCY_MANUAL;
	static const int SYNC_FREQUENCY_PUSH;

	const MojObject& GetAccountId() const { return m_accountId; }

	const MojObject& GetInboxFolderId() const { return m_inboxFolderId; }
	const MojObject& GetArchiveFolderId() const { return m_archiveFolderId; }
	const MojObject& GetDraftsFolderId() const { return m_draftsFolderId; }
	const MojObject& GetSentFolderId() const { return m_sentFolderId; }
	const MojObject& GetOutboxFolderId() const { return m_outboxFolderId; }
	const MojObject& GetSpamFolderId() const { return m_spamFolderId; }
	const MojObject& GetTrashFolderId() const { return m_trashFolderId; }
	const MojInt64	GetRevision() const { return m_rev; }

	const AccountError&	GetError() const { return m_error; }
	const RetryStatus&	GetRetry() const { return m_retry; }

	int GetSyncWindowDays() const { return m_syncWindowDays; }
	int GetSyncFrequencyMins() const { return m_syncFrequencyMins; }

	// Setters
	void SetAccountId(const MojObject& accountId) { m_accountId = accountId; }

	void SetInboxFolderId(const MojObject& id) { m_inboxFolderId = id; }
	void SetArchiveFolderId(const MojObject& id) { m_archiveFolderId = id; }
	void SetDraftsFolderId(const MojObject& id) { m_draftsFolderId = id; }
	void SetSentFolderId(const MojObject& id) { m_sentFolderId = id; }
	void SetOutboxFolderId(const MojObject& id) { m_outboxFolderId = id; }
	void SetSpamFolderId(const MojObject& id) { m_spamFolderId = id; }
	void SetTrashFolderId(const MojObject& id) { m_trashFolderId = id; }
	void SetRevision(MojInt64 rev) { m_rev = rev; }

	void SetError(const AccountError& error) { m_error = error; }
	void SetRetry(const RetryStatus& retryStatus) { m_retry = retryStatus; }

	void ClearError() { m_error = AccountError(); }
	void ClearRetry() { m_retry = RetryStatus(); }

	void SetSyncWindowDays(int windowDays) { m_syncWindowDays = windowDays; }
	void SetSyncFrequencyMins(int frequency) { m_syncFrequencyMins = frequency; }

	// Other
	bool IsManualSync() const;
	bool IsPush() const;

protected:
	// account id; not to be confused with the transport account id
	MojObject	m_accountId;

	MojObject	m_inboxFolderId;
	MojObject	m_archiveFolderId;
	MojObject	m_draftsFolderId;
	MojObject	m_sentFolderId;
	MojObject	m_outboxFolderId;
	MojObject	m_spamFolderId;
	MojObject	m_trashFolderId;
	MojInt64	m_rev;

	AccountError	m_error;

	// Retry for non-fatal connect/login failures
	RetryStatus		m_retry;

	int			m_syncWindowDays;
	int			m_syncFrequencyMins;
};

#endif /* EMAILACCOUNT_H_ */
