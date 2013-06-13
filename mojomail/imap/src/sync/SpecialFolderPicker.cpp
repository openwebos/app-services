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

#include "sync/SpecialFolderPicker.h"
#include "data/ImapFolder.h"
#include "ImapPrivate.h"

using namespace std;

bool SpecialFolderPicker::s_initialized;
vector<boost::regex> SpecialFolderPicker::s_matchInboxFolder;
vector<boost::regex> SpecialFolderPicker::s_matchDraftsFolder;
vector<boost::regex> SpecialFolderPicker::s_matchSentFolder;
vector<boost::regex> SpecialFolderPicker::s_matchTrashFolder;
vector<boost::regex> SpecialFolderPicker::s_matchArchiveFolder;
vector<boost::regex> SpecialFolderPicker::s_matchSpamFolder;

SpecialFolderPicker::SpecialFolderPicker(const vector<ImapFolderPtr>& candidates)
: m_candidates(candidates)
{
	if(!s_initialized) {
		Initialize();
	}
}

SpecialFolderPicker::~SpecialFolderPicker()
{
}

void SpecialFolderPicker::Initialize()
{	
	// Inbox
	s_matchInboxFolder.push_back( boost::regex("INBOX", boost::regex::icase) );
	
	// Drafts
	s_matchDraftsFolder.push_back( boost::regex("drafts.*", boost::regex::icase) );
	s_matchDraftsFolder.push_back( boost::regex("draft.*", boost::regex::icase) );
	s_matchDraftsFolder.push_back( boost::regex("saved.*", boost::regex::icase) );
	
	// Sent
	s_matchSentFolder.push_back( boost::regex("sent messages", boost::regex::icase) ); // used by MobileMe
	s_matchSentFolder.push_back( boost::regex("sent.*", boost::regex::icase) );
	
	// Trash
	s_matchTrashFolder.push_back( boost::regex("deleted messages", boost::regex::icase) ); // used by MobileMe
	s_matchTrashFolder.push_back( boost::regex("trash.*", boost::regex::icase) );

	// Spam
	s_matchSpamFolder.push_back( boost::regex("spam.*", boost::regex::icase) );
	s_matchSpamFolder.push_back( boost::regex("bulk mail", boost::regex::icase) );
}

bool SpecialFolderPicker::FolderIdExists(const MojObject& folderId)
{
	if(!IsValidId(folderId)) return false;

	BOOST_FOREACH(const ImapFolderPtr& folder, m_candidates) {
		if(folder->GetId() == folderId) {
			return true;
		}
	}

	return false;
}

ImapFolderPtr SpecialFolderPicker::FindBestMatch(const std::string& xlistType, const std::vector<boost::regex>& regexes)
{
	int bestScore = 100; // lowest score wins
	ImapFolderPtr bestMatch;
	
	BOOST_FOREACH(const ImapFolderPtr &folder, m_candidates) {

		if(xlistType == folder->GetXlistType()) {
			return folder;
		}

		const string& name = folder->GetDisplayName();
		
		int score = 0;
		
		// TODO: deprioritize deeply nested folders
		BOOST_FOREACH(const boost::regex& regex, regexes) {
			if(regex_match(name, regex)) {
				if(score < bestScore && m_exclude.find(folder) == m_exclude.end()) {
					bestScore = score;
					bestMatch = folder;
				}
				break;
			}
			
			// Regexes appearing earlier in the list are preferred over the later ones
			score++;
		}
	}
	
	return bestMatch;
}

ImapFolderPtr SpecialFolderPicker::MatchInbox()
{
	return FindBestMatch(ImapFolder::XLIST_INBOX, s_matchInboxFolder);
}

ImapFolderPtr SpecialFolderPicker::MatchDrafts()
{
	return FindBestMatch(ImapFolder::XLIST_DRAFTS, s_matchDraftsFolder);
}

ImapFolderPtr SpecialFolderPicker::MatchSent()
{
	return FindBestMatch(ImapFolder::XLIST_SENT, s_matchSentFolder);
}

ImapFolderPtr SpecialFolderPicker::MatchTrash()
{
	return FindBestMatch(ImapFolder::XLIST_TRASH, s_matchTrashFolder);
}

ImapFolderPtr SpecialFolderPicker::MatchArchive()
{
	return FindBestMatch(ImapFolder::XLIST_ALLMAIL, s_matchArchiveFolder);
}

ImapFolderPtr SpecialFolderPicker::MatchSpam()
{
	return FindBestMatch(ImapFolder::XLIST_SPAM, s_matchSpamFolder);
}
