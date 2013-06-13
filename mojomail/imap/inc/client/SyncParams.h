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

#ifndef SYNCPARAMS_H_
#define SYNCPARAMS_H_

#include "activity/Activity.h"
#include <boost/shared_ptr.hpp>
#include <vector>

/**
 * Stores parameters associated with a given sync request.
 *
 * SyncParams are implicitly shared.
 */
class SyncParams
{
protected:
	struct Params
	{
		Params()
		: force(false),
		  forceReconnect(false),
		  forceRebind(false),
		  syncEmails(true),
		  syncChanges(false),
		  syncFolderList(false)
		{
		}

		// Force a full sync
		bool				force;

		// String describing what triggered the sync (informative only)
		std::string			reason;

		// Activity that started the sync
		ActivityPtr					activity;

		// List of sync-related activities
		std::vector<ActivityPtr>	syncActivities;

		// List of connection-related activities
		std::vector<ActivityPtr>	connectionActivities;

		// Full payload passed to bus dispatcher method
		MojObject			payload;

		// Prefer to bind to this IP address
		// Will still use existing connection unless forceReconnect/forceRebind is set
		std::string			bindAddress;

		// Force opening a new connection (not implemented yet)
		// This should be set if the server address changes
		bool				forceReconnect;

		// Force reconnect only if interface address has changed (not used yet)
		bool				forceRebind;

		// Download new emails (also implies syncChanges)
		bool				syncEmails;

		// Upload changes
		bool				syncChanges;

		// Folder list needs to be updated (not used yet)
		bool				syncFolderList;
	};

public:
	SyncParams();
	virtual ~SyncParams();

	void SetReason(const std::string& reason) const { m_params->reason = reason; }
	const std::string& GetReason() const { return m_params->reason; }

	void SetForce(bool force) const { m_params->force = force; }
	bool GetForce() const { return m_params->force; }

	// Deprecated -- should use GetSyncActivities and GetConnectionActivities
	void SetActivity(const ActivityPtr& activity) const { m_params->activity = activity; }
	const ActivityPtr& GetActivity() const { return m_params->activity; }

	std::vector<ActivityPtr>& GetSyncActivities() const { return m_params->syncActivities; }
	std::vector<ActivityPtr>& GetConnectionActivities() const { return m_params->connectionActivities; }
	std::vector<ActivityPtr> GetAllActivities() const;

	void SetBindAddress(const std::string& address) const { m_params->bindAddress = address; }
	const std::string& GetBindAddress() const { return m_params->bindAddress; }

	void SetPayload(const MojObject& payload) const { m_params->payload = payload; }
	const MojObject& GetPayload() const { return m_params->payload; }

	void SetSyncEmails(bool sync) const { m_params->syncEmails = sync; }
	bool GetSyncEmails() const { return m_params->syncEmails; }

	void SetSyncChanges(bool sync) const { m_params->syncChanges = sync; }
	bool GetSyncChanges() const { return m_params->syncChanges; }

	void SetSyncFolderList(bool sync) const { m_params->syncFolderList = sync; }
	bool GetSyncFolderList() const { return m_params->syncFolderList; }

	void SetForceReconnect(bool reconnect) const { m_params->forceReconnect = reconnect; }
	bool GetForceReconnect() const { return m_params->forceReconnect; }

	void Status(MojObject& status) const;

protected:
	boost::shared_ptr<Params> m_params;
};

#endif /* SYNCPARAMS_H_ */
