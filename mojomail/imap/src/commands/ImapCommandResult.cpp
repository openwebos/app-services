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

#include "commands/ImapCommandResult.h"
#include "exceptions/MailException.h"
#include "exceptions/ExceptionUtils.h"
#include "ImapPrivate.h"

ImapCommandResult::ImapCommandResult()
: m_doneSignal(this)
{
}

ImapCommandResult::ImapCommandResult(MojSignal<>::SlotRef doneSlot)
: m_doneSignal(this)
{
	ConnectDoneSlot(doneSlot);
}

ImapCommandResult::~ImapCommandResult()
{
}

void ImapCommandResult::ConnectDoneSlot(MojSignal<>::SlotRef doneSlot)
{
	m_doneSignal.connect(doneSlot);
}

void ImapCommandResult::SetException(const std::exception& e)
{
	m_exception = ExceptionUtils::CopyException(e);
}

void ImapCommandResult::SetException()
{
	m_exception = ExceptionUtils::CopyException(UNKNOWN_EXCEPTION);
}

void ImapCommandResult::CheckException()
{
	if(m_exception)
		boost::rethrow_exception(m_exception);
}

void ImapCommandResult::Done()
{
	m_doneSignal.fire();
}
