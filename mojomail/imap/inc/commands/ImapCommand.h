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

#ifndef IMAPCOMMAND_H_
#define IMAPCOMMAND_H_

#include "client/Command.h"
#include <string>
#include "core/MojLogEngine.h"
#include <ctime>
#include "CommonErrors.h"

class ImapSession;
class ImapCommandResult;

#define CommandTraceFunction()						\
			MojLogTrace(ImapCommand::m_log);			\
			m_lastFunction=__PRETTY_FUNCTION__;		\

#define CATCH_AS_FAILURE catch (const std::exception& e) { Failure(e); }

class ImapCommand : public Command
{
public:
	enum CommandType {
		CommandType_Unknown,
		CommandType_Sync,
		CommandType_DownloadPart
	};

	enum CancelType {
		CancelType_Unknown,
		CancelType_NoAccount,
		CancelType_NoConnection,
		CancelType_Shutdown
	};

	ImapCommand(Listener& listener, Priority priority = NormalPriority, MojLogger& logger = *MojLogEngine::instance()->defaultLogger());
	virtual ~ImapCommand();
	
	virtual void Run();
	virtual void Cancel();
	virtual bool Cancel(CancelType cancelType);

	static MailError::ErrorInfo GetCancelErrorInfo(CancelType cancelType);
	
	bool IsRunning() { return m_running; }

	virtual void Run(MojSignal<>::SlotRef doneSlot);

	// Returns a string with a short description of the command
	virtual std::string Describe() const;

	// Store a result object which monitors the command failure/completion
	virtual void SetResult(const MojRefCountedPtr<ImapCommandResult>& result);

	virtual const MojRefCountedPtr<ImapCommandResult>& GetResult();

	virtual void Status(MojObject& status) const;

	virtual CommandType GetType() const { return CommandType_Unknown; }

	virtual bool Equals(const ImapCommand& other) const { return false; }

	/**
	 * Reports that the command is busy doing something, and therefore not stuck.
	 * Note that the command may be waiting on some other operation, like uploading to the server.
	 */
	//virtual void CommandProgress();

protected:
	virtual void	RunImpl() = 0;
	virtual void	Failure(const std::exception& exc);
	virtual void	Complete();
	virtual void	Cleanup();

	virtual bool	PrepareToRun();
	virtual void	RunIfReady();

	MojLogger&		m_log;
	const char*		m_lastFunction;

	time_t			m_createTime;
	time_t			m_startTime;

	bool			m_running;

	static MojLogger	s_log;

	MojRefCountedPtr<ImapCommandResult>		m_result;
};

#endif /*IMAPCOMMAND_H_*/
