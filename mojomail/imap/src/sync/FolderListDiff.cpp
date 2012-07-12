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

#include "sync/FolderListDiff.h"
#include <map>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;


FolderListDiff::FolderListDiff()
{
}

FolderListDiff::~FolderListDiff()
{
}

void FolderListDiff::GenerateDiff(const vector<ImapFolderPtr>& localFolders, const vector<ImapFolderPtr>& remoteFolders)
{
	map<string, ImapFolderPtr> existingFolders;
	
	// Add local folders to oldFolders
	vector<ImapFolderPtr>::const_iterator it;
	for(it = localFolders.begin(); it != localFolders.end(); ++it) {
		const ImapFolderPtr& folder = *it;

		string folderName = folder->GetFolderName();

		// Normalize "inbox" as "INBOX" for case-insensitive matching
		if(boost::iequals(folderName, ImapFolder::INBOX_FOLDER_NAME)) {
			folderName = ImapFolder::INBOX_FOLDER_NAME;
		}

		existingFolders[folderName] = folder;
	}
	
	// For each remote folder, find a matching local folder (if any)
	for(it = remoteFolders.begin(); it != remoteFolders.end(); ++it) {
		const ImapFolderPtr& remoteFolder = *it;
		
		// Special case for inbox, which is case-insensitive
		string folderName = remoteFolder->GetFolderName();
		if(remoteFolder->GetXlistType() == ImapFolder::XLIST_INBOX || boost::iequals(folderName, ImapFolder::INBOX_FOLDER_NAME)) {
			folderName = ImapFolder::INBOX_FOLDER_NAME;
		}

		map<string, ImapFolderPtr>::iterator localMatch;
		localMatch = existingFolders.find(folderName);
		
		if(localMatch != existingFolders.end()) {
			// add local and remote folder to map
			ImapFolderPtr& localFolder = localMatch->second;
			m_folderMap[localFolder] = remoteFolder;
			
			// add to folder list
			m_folders.push_back(localFolder);
			
			// remove entry from local-only folder list
			existingFolders.erase(localMatch);
		} else {
			// no existing folder; new folder
			m_newFolders.push_back(remoteFolder);
			
			m_folders.push_back(remoteFolder);
		}
	}
	
	// Any folders left over are deleted folders
	map<string, ImapFolderPtr>::iterator existingIt;
	for(existingIt = existingFolders.begin(); existingIt != existingFolders.end(); ++existingIt) {
		m_deletedFolders.push_back(existingIt->second);
	}
}
