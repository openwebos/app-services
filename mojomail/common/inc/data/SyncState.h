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

#ifndef SYNCSTATE_H_
#define SYNCSTATE_H_

#include "core/MojObject.h"
#include <string>

class SyncState
{
public:
	SyncState(const MojObject& accountId);
	SyncState(const MojObject& accountId, const MojObject& folderId);

	virtual ~SyncState();

	enum State {
		IDLE,
		PUSH,
		INITIAL_SYNC,
		INCREMENTAL_SYNC,
		DELETE,
		ERROR
	};

	void SetState(State syncState) { m_syncState = syncState; }
	void SetError(const std::string& errorCode, const std::string& errorText = "") { m_errorCode = errorCode; m_errorText = errorText; }

	const char* GetErrorCode() const { return m_errorCode.c_str();}
	const char* GetErrorText() const { return m_errorText.c_str();}

	const MojObject& GetAccountId() const { return m_accountId; }
	const MojObject& GetCollectionId() const { return m_collectionId; }

	State GetState() const { return m_syncState; }

protected:
	MojObject		m_accountId;
	MojObject		m_collectionId;

	State			m_syncState;
	std::string		m_errorCode;
	std::string		m_errorText;
};

#endif /* SYNCSTATE_H_ */
