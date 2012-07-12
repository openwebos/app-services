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

#include "commands/PopCommandResult.h"
#include "exceptions/MailException.h"
#include "exceptions/ExceptionUtils.h"

PopCommandResult::PopCommandResult()
: m_doneSignal(this)
{
}

PopCommandResult::PopCommandResult(MojSignal<>::SlotRef doneSlot)
: m_doneSignal(this)
{
	ConnectDoneSlot(doneSlot);
}

PopCommandResult::~PopCommandResult()
{
}

void PopCommandResult::ConnectDoneSlot(MojSignal<>::SlotRef doneSlot)
{
	m_doneSignal.connect(doneSlot);
}

void PopCommandResult::SetException(const std::exception& e)
{
	m_exception = ExceptionUtils::CopyException(e);
}

void PopCommandResult::SetException()
{
	m_exception = ExceptionUtils::CopyException(boost::unknown_exception());
}

void PopCommandResult::CheckException()
{
	if(m_exception)
		boost::rethrow_exception(m_exception);
}

void PopCommandResult::Done()
{
	m_doneSignal.fire();
}
