// @@@LICENSE
//
//      Copyright (c) 2009-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef POPCOMMAND_H_
#define POPCOMMAND_H_

#include "client/Command.h"
#include "commands/PopCommandResult.h"
#include <string>

class PopSession;
class PopCommandResult;

// TODO:  Test to see if it is actually working.
#define CommandTraceFunction()						\
			MojLogTrace(PopCommand::m_log);			\
			m_lastFunction=__PRETTY_FUNCTION__;		\

#define CATCH_AS_FAILURE catch (const std::exception& e) { Failure(e); }

class PopCommand : public Command
{
public:
	PopCommand(Listener& listener, Priority priority = NormalPriority);
	virtual ~PopCommand();

	virtual void Run();
	virtual void Cancel();

	virtual void Run(MojSignal<>::SlotRef doneSlot);

	// Store a result object which monitors the command failure/completion
	virtual void SetResult(const MojRefCountedPtr<PopCommandResult>& result);

	virtual const MojRefCountedPtr<PopCommandResult>& GetResult();

	virtual void Status(MojObject& status) const;

protected:
	virtual void	RunImpl() = 0;
	virtual void	Failure(const std::exception& exc) = 0;

	virtual bool	PrepareToRun();
	virtual void	RunIfReady();

	/**
	 * Clean up resources that this command allocates.
	 */
	virtual void 	Cleanup();

	static MojLogger s_log;

	MojLogger&		m_log;
	const char*		m_lastFunction;
	bool			m_isCancelled;

	MojRefCountedPtr<PopCommandResult>		m_result;
};

#endif /* POPCOMMAND_H_ */
