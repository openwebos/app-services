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

#ifndef IMAPEMAIL_H_
#define IMAPEMAIL_H_

#include "data/Email.h"
#include "ImapCoreDefs.h"

struct EmailFlags
{
	EmailFlags() : read(false), replied(false), flagged(false) {}

	inline bool operator==(const EmailFlags& other)
	{
		return read == other.read && replied == other.replied && flagged == other.flagged;
	}

	inline bool operator!=(const EmailFlags& other)
	{
		return !(*this == other);
	}

	unsigned int read		: 1;
	unsigned int replied	: 1;
	unsigned int flagged	: 1;
};

class ImapEmail : public Email
{
public:
	ImapEmail();
	virtual ~ImapEmail();

	void SetUID(UID uid) { m_uid = uid; }
	UID GetUID() const { return m_uid; }

	void SetDeleted(bool deleted) { m_deleted = deleted; }
	bool IsDeleted() const { return m_deleted; }

	void SetAutoDownload(bool autoDownload) { m_autoDownload = autoDownload; }
	bool IsAutoDownload() const { return m_autoDownload; }

protected:
	UID		m_uid;
	bool	m_deleted;
	bool	m_autoDownload;
};


#endif /*IMAPEMAIL_H_*/
