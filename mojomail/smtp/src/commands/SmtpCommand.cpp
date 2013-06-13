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

#include "SmtpBusDispatcher.h"
#include "SmtpClient.h"
#include "commands/SmtpCommand.h"

using namespace std;

SmtpCommand::SmtpCommand(Listener& client, Priority priority)
: Command(client, priority),
  m_log(SmtpBusDispatcher::s_log)
{
	assert( &client );
}

SmtpCommand::~SmtpCommand()
{
}

void SmtpCommand::Run()
{
	fprintf(stderr, "Running command, m_listener = %p\n", &m_listener);

	try {
		m_stats.m_started = time(NULL);

		RunImpl();
	} catch(const exception& e) {
		Failure(e);
	} catch(...) {
		Failure(MailException("unknown error", __FILE__, __LINE__));
	}
}

void SmtpCommand::Failure(const exception& exc)
{
	MojLogWarning(m_log, "exception in command %s: %s", GetClassName().c_str(), exc.what());
	Complete();
}

void SmtpCommand::Cancel()
{
	// Note: most SMTP protocol commands can't be cancelled while in progress.
	// Cancel could still be used for cancelling opening a connection.
	// Cancelling attachment downloads could be implemented by closing the socket.
}

void SmtpCommand::Status(MojObject& status) const
{
	MojErr err;
	Command::Status(status);

	if(m_stats.m_started > 0) {
		err = status.put("startTime", (MojInt64) m_stats.m_started);
		ErrorToException(err);
		err = status.put("timeRunning", (MojInt64) (time(NULL) - m_stats.m_started));
		ErrorToException(err);
	}

	if(m_stats.m_lastFunction) {
		err = status.putString("lastFunction", m_stats.m_lastFunction);
		ErrorToException(err);
	}
}
