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

#ifndef SPECIALFOLDERPICKER_H_
#define SPECIALFOLDERPICKER_H_

#include <boost/regex.hpp>
#include <boost/shared_ptr.hpp>
#include <vector>
#include <set>
#include "ImapCoreDefs.h"

class ImapFolder;

class SpecialFolderPicker
{
public:
	SpecialFolderPicker(const std::vector<ImapFolderPtr>& candidates);
	virtual ~SpecialFolderPicker();

	bool FolderIdExists(const MojObject& folderId);

	ImapFolderPtr MatchInbox();
	ImapFolderPtr MatchDrafts();
	ImapFolderPtr MatchSent();
	ImapFolderPtr MatchTrash();
	ImapFolderPtr MatchArchive();
	ImapFolderPtr MatchSpam();
	
protected:
	static void Initialize();
	
	static bool s_initialized;
	
	static std::vector<boost::regex> s_matchInboxFolder;
	static std::vector<boost::regex> s_matchDraftsFolder;
	static std::vector<boost::regex> s_matchSentFolder;
	static std::vector<boost::regex> s_matchTrashFolder;
	static std::vector<boost::regex> s_matchArchiveFolder;
	static std::vector<boost::regex> s_matchSpamFolder;
	
	ImapFolderPtr FindBestMatch(const std::string& xlistType, const std::vector<boost::regex>& regexes);
	
	const std::vector<ImapFolderPtr>& m_candidates;
	std::set<ImapFolderPtr> m_exclude;
};

#endif /*SPECIALFOLDERPICKER_H_*/
