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

#ifndef COMMAND_H_
#define COMMAND_H_

#include "core/MojSignal.h"
#include <string>

class Command : public MojSignalHandler
{
public:
	class Listener
	{
	public:
		virtual void CommandComplete(Command* command) = 0;
	};

	typedef enum {
		LowPriority		= 1000,
		NormalPriority	= 2000,
		HighPriority 	= 3000
	} Priority;

	Command(Listener& listener, Priority priority) : m_listener(listener), m_priority(priority), m_commandNum(0) { }
	virtual ~Command() { }

	virtual void Run() = 0;
	virtual void Cancel() = 0;
	virtual void Complete() { m_listener.CommandComplete(this); }

	Priority GetPriority() const { return m_priority; }

	bool operator<(const Command& b) const { return GetPriority() < b.GetPriority(); }

	// Returns a string with a short description of the command
	virtual std::string Describe() const { return GetClassName(); }

	// Returns the name of the command
	std::string GetClassName() const;

	// Report command status
	virtual void Status(MojObject& status) const;

	// Set the command number used by the CommandManager to order commands of the same priority
	virtual void SetCommandNumber(unsigned int num) { m_commandNum = num; };
	virtual unsigned int GetCommandNumber() { return m_commandNum; };

protected:
	Listener& m_listener;
	Priority m_priority;
	unsigned int m_commandNum;
};

#endif /* COMMAND_H_ */
