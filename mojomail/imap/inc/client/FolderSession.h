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

#ifndef FOLDERSESSION_H_
#define FOLDERSESSION_H_

#include "ImapCoreDefs.h"

class UIDMap;

/**
 * Class for tracking the state associated with a selected folder.
 */
class FolderSession
{
public:
	FolderSession(const ImapFolderPtr& folder);
	virtual ~FolderSession();

	const MojObject&		GetFolderId() const;
	const ImapFolderPtr&	GetFolder() const { return m_folder; }

	int		GetMessageCount() { return m_count; }

	bool	HasUIDMap() const { return m_uidMap.get(); }

	const boost::shared_ptr<UIDMap>&	GetUIDMap() const { return m_uidMap; }

	void	SetUIDMap(const boost::shared_ptr<UIDMap>& uidMap) { m_uidMap = uidMap; }
	void	SetMessageCount(unsigned int count) { m_count = count; }

protected:
	// Database folder currently selected
	ImapFolderPtr				m_folder;

	// Number of messages when we selected the folder
	int							m_count;

	// UIDMap for associating message sequence numbers with UIDs
	boost::shared_ptr<UIDMap>	m_uidMap;
};

#endif /* FOLDERSESSION_H_ */
