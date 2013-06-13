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

#ifndef COMMANDMANAGER_H_
#define COMMANDMANAGER_H_

#include "client/Command.h"
#include "core/MojRefCount.h"
#include <glib.h>
#include <memory>
#include <queue>
#include <vector>

/**
 * \brief
 * The CommandManager is responsible for the lifecycle of all Commands.
 */
class CommandManager : public MojRefCounted, public Command::Listener
{
public:
	typedef MojRefCountedPtr<Command> CommandPtr;
	typedef std::vector<CommandPtr>::const_iterator CommandConstIterator;

	static const int DEFAULT_MAX_CONCURRENT_COMMANDS = 4;

	CommandManager(size_t maxConcurrentCommands = DEFAULT_MAX_CONCURRENT_COMMANDS, bool startPaused = false);
	virtual ~CommandManager();

	/**
	 * Adds a command to the queue.
	 * @param the command to add
	 * @param if true, the ScheduleRunCommands() will be invoked immediately after
	 */
	virtual void QueueCommand(CommandPtr command, bool runImmediately = true);

	/**
	 * Schedules a Glib event to call RunCommands() when the main loop is idle.
	 */
	virtual void ScheduleRunCommands();

	/**
	 * Run eligible commands pending in the queue.
	 */
	virtual void RunCommands();

	/**
	 * Runs command immediately (does not queue).
	 */
	virtual void RunCommand(CommandPtr command);

	/**
	 * Called by a Command when it is done executing.
	 * This will move the command to the completed commands list and mark it for deletion.
	 */
	virtual void CommandComplete(Command* command);

	/**
	 * Deletes all completed commands.
	 */
	virtual void Cleanup();

	/**
	 * Prevents CommandManger from running commands in the queue automatically
	 * after a command completes.
	 */
	virtual void Pause();

	/**
	 * Allows CommandManager to resume running commands automatically
	 * after a command completes.
	 */
	virtual void Resume();

	/**
	 * Returns the number of pending commands.
	 */
	virtual int GetPendingCommandCount();

	/**
	 * Returns the number of active commands.
	 */
	virtual int GetActiveCommandCount();

	/**
	 * Fills in a MojObject with the command manager status
	 */
	virtual void Status(MojObject& status) const;

	/**
	 * @returns the next command to run (i.e. the command in the front of the queue)
	 */
	virtual CommandPtr Top() const;

	/**
	 * Removes the next command (the command in front of the queue) from the queue.
	 */
	virtual void Pop();

	/**
	 * Get begin and end iterator for pending commands.
	 * Be careful not to call any other CommandManager functions otherwise the iterators may become invalid.
	 */
	std::pair<CommandConstIterator, CommandConstIterator> GetPendingCommandIterators() const
	{
		return std::make_pair(m_pendingCommands.begin(), m_pendingCommands.end());
	}

	/**
	 * Get begin and end iterator for active commands.
	 * Be careful not to call any other CommandManager functions otherwise the iterators may become invalid.
	 */
	std::pair<CommandConstIterator, CommandConstIterator> GetActiveCommandIterators() const
	{
		return std::make_pair(m_activeCommands.begin(), m_activeCommands.end());
	}

protected:
	typedef std::vector<CommandPtr>			CommandVec;

	struct CompareCommandPtrs : public std::less<CommandPtr>
	{
		bool operator()(const CommandPtr& c1, const CommandPtr& c2) const
		{
			if ( !(*c1 < *c2) && !(*c2 < *c1) ) // equal priority if neither is less than the other
				return c1->GetCommandNumber() > c2->GetCommandNumber();
			else
				return *c1 < *c2;
		}
	};

	class CommandQueue : public std::priority_queue<CommandPtr, std::vector<CommandPtr>, CompareCommandPtrs >
	{
	public:
		container_type::iterator begin() { return c.begin(); };
		container_type::iterator end() { return c.end(); };
		container_type::const_iterator begin() const { return c.begin(); };
		container_type::const_iterator end() const { return c.end(); };
		container_type::iterator erase(container_type::iterator it) { return c.erase(it); };
	};

	CommandQueue	m_pendingCommands;
	CommandVec		m_activeCommands;
	CommandVec		m_completedCommands;

	size_t	m_maxConcurrentCommands;
	bool	m_paused;

	guint	m_runCallbackId;
	guint	m_cleanupCallbackId;

private:
	static MojLogger s_log;

	/**
	 * GLib Main Event Loop callback to delete any completed commands.
	 */
	static gboolean CleanupCallback(gpointer data);

	/**
	 * GLib Main Event Loop callback to run eligible pending commands.
	 */
	static gboolean RunCallback(gpointer data);
};

#endif /* COMMANDMANAGER_H_ */
