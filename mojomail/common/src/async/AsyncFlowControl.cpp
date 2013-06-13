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

#include "async/AsyncFlowControl.h"
#include "email/AsyncEmailWriter.h"
#include "async/AsyncOutputStream.h"

AsyncFlowControl::AsyncFlowControl(const MojRefCountedPtr<AsyncEmailWriter>& writer, const MojRefCountedPtr<AsyncOutputStream>& sink)
: m_writer(writer),
  m_sink(sink),
  m_sinkFullSlot(this, &AsyncFlowControl::HandleSinkFull),
  m_sinkWriteableSlot(this, &AsyncFlowControl::HandleSinkWriteable)
{
	m_sink->SetFullSlot(m_sinkFullSlot);
	m_sink->SetWriteableSlot(m_sinkWriteableSlot);
}

AsyncFlowControl::~AsyncFlowControl()
{
}

MojErr AsyncFlowControl::HandleSinkFull()
{
	m_writer->PauseWriting();

	return MojErrNone;
}

MojErr AsyncFlowControl::HandleSinkWriteable()
{
	m_writer->ResumeWriting();

	return MojErrNone;
}
