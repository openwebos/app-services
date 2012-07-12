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

#ifndef FOLDERLISTDIFF_H_
#define FOLDERLISTDIFF_H_

#include <boost/shared_ptr.hpp>
#include <vector>
#include <map>
#include "data/ImapFolder.h"

class FolderListDiff
{
public:
	FolderListDiff();
	virtual ~FolderListDiff();
	
	void GenerateDiff(const std::vector<ImapFolderPtr>& localFolders, const std::vector<ImapFolderPtr>& remoteFolders);
	
	// Get folders that exist only on the server. Will need to assign them an accountId.
	const std::vector<ImapFolderPtr>& GetNewFolders() const { return m_newFolders; }
	
	// Get folders that no longer exist on the server.
	const std::vector<ImapFolderPtr>& GetDeletedFolders() const { return m_deletedFolders; }
	
	// Get mapping from local to remote folders.
	const std::map<ImapFolderPtr, ImapFolderPtr> GetFolderMap() const { return m_folderMap; }
	
	// Get list of all non-deleted folders, consisting of existing and new folders.
	// Note that new folders may not have an id yet.
	const std::vector<ImapFolderPtr>& GetFolders() const { return m_folders; }

protected:
	std::vector<ImapFolderPtr> m_newFolders;
	std::vector<ImapFolderPtr> m_deletedFolders;
	std::map<ImapFolderPtr, ImapFolderPtr> m_folderMap;
	std::vector<ImapFolderPtr> m_folders;
};

#endif /*FOLDERLISTDIFF_H_*/
