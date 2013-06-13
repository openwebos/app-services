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

#ifndef SMTPCOMMAND_H_
#define SMTPCOMMAND_H_

#include <SmtpCommon.h>
#include "client/Command.h"
#include <ctime>

#define CommandTraceFunction()									\
			m_stats.m_lastFunction = __PRETTY_FUNCTION__

class SmtpCommand : public Command
{
public:
	// The status of this current command.
	class CommandStats
	{
	public:
		CommandStats() : m_created(time(NULL)), m_started(0), m_lastFunction(NULL) {}

		//CommandType m_type;
		time_t m_created;
		time_t m_started;
		const char* m_lastFunction;
	};
	
	void Failure(const exception& exc);
	void Cancel();

	SmtpCommand(Listener& client, Priority priority = NormalPriority);
	virtual ~SmtpCommand();
	
	virtual void Run();
	virtual void RunImpl() = 0;

	virtual void Status(MojObject& status) const;
         	
protected:
	MojLogger& m_log;
	CommandStats		m_stats;
};

#endif /*SMTPCOMMAND_H_*/
