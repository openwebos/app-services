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

#ifndef IMAPSESSIONCOMMAND_H_
#define IMAPSESSIONCOMMAND_H_

#include "commands/ImapCommand.h"
#include <string>
#include <sstream>
#include "exceptions/MailException.h"
#include "activity/Activity.h"

class ActivitySet;

class ImapSessionCommand : public ImapCommand
{
public:
	ImapSessionCommand(ImapSession& session, Priority priority = NormalPriority);
	virtual ~ImapSessionCommand();
	
	static std::string QuoteString(const std::string& s);

	void AddActivity(const ActivityPtr& activity);

	virtual void Status(MojObject& status) const;

	virtual std::string Describe() const;
	std::string Describe(const MojObject& folderId) const;

protected:

	template<class InputIterator>
	static void AppendUIDs(std::stringstream& ss, const InputIterator& begin, const InputIterator& end)
	{
		if(begin != end) {
			for(InputIterator it = begin; it != end; ++it) {
				if(it != begin) {
					ss << ",";
				}
				ss << *it;
			}
		} else {
			throw MailException("empty UID list", __FILE__, __LINE__);
		}
	}

	virtual bool PrepareToRun();

	virtual MojErr CommandActivityStarted();

	virtual void Cleanup();

	ImapSession&	m_session;

	MojRefCountedPtr<ActivitySet>	m_commandActivitySet;

private:
	bool			m_readyToRun;

	MojSignal<>::Slot<ImapSessionCommand> m_startActivitiesSlot;
};

#endif /*IMAPSESSIONCOMMAND_H_*/
