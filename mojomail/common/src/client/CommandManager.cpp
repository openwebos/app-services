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

#include "client/Command.h"
#include "client/CommandManager.h"
#include "CommonPrivate.h"

MojLogger CommandManager::s_log("com.palm.mail.commandmanager");

CommandManager::CommandManager(size_t maxConcurrentCommands, bool startPaused)
: m_maxConcurrentCommands(maxConcurrentCommands),
  m_paused(startPaused),
  m_runCallbackId(0),
  m_cleanupCallbackId(0)
{

}

CommandManager::~CommandManager()
{
	if(m_runCallbackId > 0) {
		g_source_remove(m_runCallbackId);
	}

	if(m_cleanupCallbackId > 0) {
		g_source_remove(m_cleanupCallbackId);
	}
}

void CommandManager::QueueCommand(CommandPtr command, bool runImmediately)
{
	// TODO: check for similar commands already in the queue

	// Set the command number so we can ensure FIFO if they have the same priority
	static unsigned int nextCommandNumber = 0;
	command->SetCommandNumber(nextCommandNumber++);

	m_pendingCommands.push(command);

	if (runImmediately)
		ScheduleRunCommands();
}

void CommandManager::ScheduleRunCommands()
{
	// Schedule an event to execute the next set of commands once the stack is unwound.
	// This prevents us from running commands on the same stack as a completed command.
	if(!m_paused && m_runCallbackId == 0) {
		m_runCallbackId = g_idle_add(&CommandManager::RunCallback, this);
	}
}

void CommandManager::RunCommands()
{
	if(!m_paused) {
		while (m_activeCommands.size() < m_maxConcurrentCommands && !m_pendingCommands.empty()) {
			CommandPtr command = m_pendingCommands.top();
			m_pendingCommands.pop();
			RunCommand(command);
		}
	}
}

void CommandManager::RunCommand(CommandPtr command)
{
	MojLogDebug(s_log, "running command: %i", (int) command.get());
	m_activeCommands.push_back(command);
	command->Run();
}

void CommandManager::CommandComplete(Command* command)
{
	bool found = false;

	// Delete the active command
	for (CommandVec::iterator it = m_activeCommands.begin(); it < m_activeCommands.end(); it++) {
		if (it->get() == command) {
			found = true;
			CommandPtr completed = *it;
			m_activeCommands.erase(it);
			m_completedCommands.push_back(completed);
			MojLogDebug(s_log, "command completed: %i", (int) command);
			break;
		}
	}

	if (!found) {
		// If it's not found in the active list, the command might still be pending
		for (CommandQueue::container_type::iterator it = m_pendingCommands.begin(); it < m_pendingCommands.end(); it++) {
			if (it->get() == command) {
				CommandPtr completed = *it;
				m_pendingCommands.erase(it);
				m_completedCommands.push_back(completed);
				MojLogDebug(s_log, "aborted pending command: %i", (int) command);
				break;
			}
		}
	}

	if (m_completedCommands.size() == 1) {
		m_cleanupCallbackId = g_idle_add_full(G_PRIORITY_HIGH, &CleanupCallback, this, NULL);
	}

	// Now check the queue to see if we have more work to do
	if(!m_paused)
		ScheduleRunCommands();
}

void CommandManager::Pause()
{
	m_paused = true;

	// TODO remove scheduled callback?
}

void CommandManager::Resume()
{
	m_paused = false;
	ScheduleRunCommands();
}

int CommandManager::GetPendingCommandCount()
{
	return m_pendingCommands.size();
}

int CommandManager::GetActiveCommandCount()
{
	return m_activeCommands.size();
}

void CommandManager::Cleanup()
{
	CommandVec::iterator it;
	for (it = m_completedCommands.begin(); it < m_completedCommands.end(); it++) {
		MojLogDebug(s_log, "deleting command: %i", (int) it->get());
	}
	m_completedCommands.clear();
}

gboolean CommandManager::RunCallback(gpointer data)
{
	assert(data);
	CommandManager* manager = static_cast<CommandManager*>(data);

	try {
		manager->m_runCallbackId = 0;
		manager->RunCommands();
	} catch (const std::exception& e) {
		MojLogCritical(s_log, "%s exception: %s", __PRETTY_FUNCTION__, e.what());
	} catch (...) {
		MojLogCritical(s_log, "%s uncaught exception", __PRETTY_FUNCTION__);
	}

	return false; // return false to make sure we don't get called again
}

gboolean CommandManager::CleanupCallback(gpointer data)
{
	assert(data);
	CommandManager* manager = static_cast<CommandManager*>(data);

	try {
		manager->m_cleanupCallbackId = 0;
		manager->Cleanup();
	} catch (const std::exception& e) {
		MojLogCritical(s_log, "%s exception: %s", __PRETTY_FUNCTION__, e.what());
	} catch (...) {
		MojLogCritical(s_log, "%s uncaught exception", __PRETTY_FUNCTION__);
	}

	return false; // return false to make sure we don't get called again
}

void CommandManager::Status(MojObject& status) const
{
	MojErr err;

	MojObject pendingCommands(MojObject::TypeArray);
	MojObject activeCommands(MojObject::TypeArray);

	for (CommandQueue::container_type::const_iterator it = m_pendingCommands.begin(); it < m_pendingCommands.end(); ++it) {
		MojObject commandStatus;
		(*it)->Status(commandStatus);
		pendingCommands.push(commandStatus);
	}

	for (CommandVec::const_iterator it = m_activeCommands.begin(); it < m_activeCommands.end(); ++it) {
		MojObject commandStatus;
		(*it)->Status(commandStatus);
		activeCommands.push(commandStatus);
	}

	err = status.put("pendingCommands", pendingCommands);
	ErrorToException(err);
	err = status.put("activeCommands", activeCommands);
	ErrorToException(err);
}

CommandManager::CommandPtr CommandManager::Top() const
{
	return m_pendingCommands.top();
}

void CommandManager::Pop()
{
	m_pendingCommands.pop();
}
