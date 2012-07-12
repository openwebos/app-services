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

#ifndef POPSESSIONCOMMAND_H_
#define POPSESSIONCOMMAND_H_

#include <boost/exception_ptr.hpp>
#include <string>
#include "commands/PopCommand.h"
#include "client/PopSession.h"
#include "PopDefs.h"

class PopSession;

class PopSessionCommand : public PopCommand
{
public:
	PopSessionCommand(PopSession& session, Priority priority = NormalPriority);
	virtual ~PopSessionCommand();

	virtual void Run();

protected:
	virtual void RunImpl() = 0;
	virtual void Complete();
	virtual void Failure(const std::exception& exc);
	virtual void NetworkFailure(MailError::ErrorCode errCode, const std::exception& ex);

	PopSession&				m_session;
	MojLogger&				m_log;
};

#endif /* POPSESSIONCOMMAND_H_ */
