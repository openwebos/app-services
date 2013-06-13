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

#include <iostream>
#include "client/CommandManager.h"
#include <gtest/gtest.h>

class MockListener : public Command::Listener
{
public:
	MockListener() { };
	virtual ~MockListener() { };

	virtual void CommandComplete(Command* command) { };
};

class MockCommand : public Command
{
public:
	MockCommand(Command::Listener& listener, Priority priority) : Command(listener, priority) { };
	virtual ~MockCommand() { };

	virtual void Run() { };
	virtual void Cancel() { };
};

TEST(CommandManagerTest, TestFifo)
{
	MockListener listener;

	MojRefCountedPtr<CommandManager> managerRef(new CommandManager(1));
	CommandManager& manager = *managerRef;

	CommandManager::CommandPtr c1(new MockCommand(listener, Command::NormalPriority));
	CommandManager::CommandPtr c2(new MockCommand(listener, Command::NormalPriority));
	CommandManager::CommandPtr c3(new MockCommand(listener, Command::NormalPriority));

	manager.QueueCommand(c1, false);
	manager.QueueCommand(c2, false);
	manager.QueueCommand(c3, false);

	CommandManager::CommandPtr dequeued = manager.Top();
	ASSERT_EQ(c1.get(), dequeued.get());
	manager.Pop();

	dequeued = manager.Top();
	ASSERT_EQ(c2.get(), dequeued.get());
	manager.Pop();

	dequeued = manager.Top();
	ASSERT_EQ(c3.get(), dequeued.get());
	manager.Pop();
}

TEST(CommandManagerTest, TestPriorities)
{
	MockListener listener;

	MojRefCountedPtr<CommandManager> managerRef(new CommandManager(1));
	CommandManager& manager = *managerRef;

	CommandManager::CommandPtr c1(new MockCommand(listener, Command::HighPriority));
	CommandManager::CommandPtr c2(new MockCommand(listener, Command::NormalPriority));
	CommandManager::CommandPtr c3(new MockCommand(listener, Command::LowPriority));
	CommandManager::CommandPtr c4(new MockCommand(listener, Command::NormalPriority));
	CommandManager::CommandPtr c5(new MockCommand(listener, Command::HighPriority));

	manager.QueueCommand(c1, false);
	manager.QueueCommand(c2, false);
	manager.QueueCommand(c3, false);
	manager.QueueCommand(c4, false);
	manager.QueueCommand(c5, false);

	CommandManager::CommandPtr dequeued = manager.Top();
	ASSERT_EQ(c1.get(), dequeued.get());
	manager.Pop();

	dequeued = manager.Top();
	ASSERT_EQ( c5.get(), dequeued.get() );
	manager.Pop();

	dequeued = manager.Top();
	ASSERT_EQ( c2.get(), dequeued.get() );
	manager.Pop();

	dequeued = manager.Top();
	ASSERT_EQ( c4.get(), dequeued.get() );
	manager.Pop();

	dequeued = manager.Top();
	ASSERT_EQ( c3.get(), dequeued.get() );
	manager.Pop();
}
