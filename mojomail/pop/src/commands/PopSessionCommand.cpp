// @@@LICENSE
//
//      Copyright (c) 2009-2013 LG Electronics, Inc.
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

#include "client/PopSession.h"
#include "commands/PopSessionCommand.h"
#include "exceptions/ExceptionUtils.h"
#include "CommonErrors.h"

using namespace std;

PopSessionCommand::PopSessionCommand(PopSession& session, Priority priority)
: PopCommand(session, priority),
  m_session(session),
  m_log(session.GetLogger())
{
	assert( &session );
}

PopSessionCommand::~PopSessionCommand()
{
}

void PopSessionCommand::Run()
{
	try {
		RunImpl();
	} catch (const MailNetworkTimeoutException& nex) {
		NetworkFailure(MailError::CONNECTION_TIMED_OUT, nex);
	} catch (const MailNetworkDisconnectionException& nex) {
		NetworkFailure(MailError::NO_NETWORK, nex);
	} catch(const exception& e) {
		Failure(e);
	} catch(...) {
		Failure(MailException("Unknow exception", __FILE__, __LINE__));
	}
}

void PopSessionCommand::Complete()
{
	// clean up command's resource and them completes the command
	Cleanup();
	Command::Complete();

	MojRefCountedPtr<PopCommandResult> result = GetResult();
	result->Done();
}

void PopSessionCommand::Failure(const exception& exc)
{
	MojLogError(m_log, "Exception in command %s: %s", Describe().c_str(), exc.what());
	Cleanup();
	m_session.CommandFailed(this, MailError::INTERNAL_ERROR, exc);

	MojRefCountedPtr<PopCommandResult> result = GetResult();
	result->SetException(exc);
	result->Done();
}

void PopSessionCommand::NetworkFailure(MailError::ErrorCode errCode, const std::exception& ex)
{
	m_session.ConnectFailure(errCode, ex.what());
	Failure(ex);
}
