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

#ifndef IMAPFOLDER_H_
#define IMAPFOLDER_H_

#include "data/Folder.h"
#include "ImapCoreDefs.h"
#include <string>

class ImapFolder : public Folder
{
public:
	ImapFolder();
	virtual ~ImapFolder();

	void SetFolderName(const std::string& folderName) { m_folderName = folderName; }
	void SetDelimiter(const std::string& delimiter) { m_delimiter = delimiter; }
	void SetUIDValidity(UID validity) { m_uidValidity = validity; }
	
	const std::string& GetFolderName() const { return m_folderName; }
	const std::string& GetDelimiter() const { return m_delimiter; }
	UID GetUIDValidity() const { return m_uidValidity; }

	void SetSelectable(bool selectable) { m_selectable = selectable; }
	bool IsSelectable() const { return m_selectable; }

	void SetXlistType(const std::string& type) { m_xlistType = type; }
	const std::string& GetXlistType() const { return m_xlistType; }

	static const char* const INBOX_FOLDER_NAME;

	static const char* const XLIST_ALLMAIL;
	static const char* const XLIST_INBOX;
	static const char* const XLIST_SENT;
	static const char* const XLIST_SPAM;
	static const char* const XLIST_TRASH;
	static const char* const XLIST_DRAFTS;

protected:
	std::string m_folderName;
	std::string m_delimiter;
	UID			m_uidValidity;
	UID			m_uidNext;

	bool		m_selectable;
	std::string	m_xlistType;	// not persisted
};

#endif /*IMAPFOLDER_H_*/
