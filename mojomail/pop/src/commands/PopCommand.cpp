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

#include "commands/PopCommand.h"
#include "client/PopSession.h"
#include "commands/PopCommandResult.h"
#include <sstream>
#include "client/SyncSession.h"

using namespace std;

MojLogger PopCommand::s_log("com.palm.pop.client");

PopCommand::PopCommand(Listener& listener, Command::Priority priority)
: Command(listener, priority),
  m_log(s_log),
  m_lastFunction(NULL),
  m_isCancelled(false)
{
}

PopCommand::~PopCommand()
{
}

bool PopCommand::PrepareToRun()
{
	if (m_isCancelled) {
		return false;
	} else {
		return true;
	}
}

void PopCommand::RunIfReady()
{
	if(PrepareToRun()) {
		RunImpl();
	}
}

void PopCommand::Run()
{
	MojLogInfo(m_log, "running command %s", Describe().c_str());

	try {
		RunIfReady();
	} catch(const exception& e) {
		Failure(e);
	} catch(...) {
		Failure(boost::unknown_exception());
	}
}

const MojRefCountedPtr<PopCommandResult>& PopCommand::GetResult()
{
	if(m_result.get()) {
		return m_result;
	} else {
		m_result.reset(new PopCommandResult());
		return m_result;
	}
}

void PopCommand::Run(MojSignal<>::SlotRef slot)
{
	if(m_result.get()) {
		m_result->ConnectDoneSlot(slot);
	} else {
		m_result.reset(new PopCommandResult(slot));
	}

	Run();
}

void PopCommand::SetResult(const MojRefCountedPtr<PopCommandResult>& result)
{
	m_result = result;
}

void PopCommand::Cleanup()
{

}

void PopCommand::Cancel()
{
	m_isCancelled = true;

	// No need to remove command from command manager.  The command should be
	// taken out of comamnd queue before it is cancelled.
	Cleanup();
}

void PopCommand::Status(MojObject& status) const
{
	MojErr err;

	err = status.putString("class", Describe().c_str());
	ErrorToException(err);

	if(m_lastFunction != NULL) {
		err = status.putString("lastFunction", m_lastFunction);
		ErrorToException(err);
	}

	if(m_isCancelled) {
		err = status.put("cancelled", true);
		ErrorToException(err);
	}
}


