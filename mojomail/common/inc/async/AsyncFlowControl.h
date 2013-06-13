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

#ifndef ASYNCFLOWCONTROL_H_
#define ASYNCFLOWCONTROL_H_

#include "core/MojSignal.h"

class AsyncEmailWriter;
class AsyncOutputStream;

// Manages flow between an email writer and output stream to prevent the output stream buffer from using unbounded memory
class AsyncFlowControl : public MojSignalHandler
{
public:
	AsyncFlowControl(const MojRefCountedPtr<AsyncEmailWriter>& writer, const MojRefCountedPtr<AsyncOutputStream>& sink);
	virtual ~AsyncFlowControl();

protected:
	MojErr HandleSinkFull();
	MojErr HandleSinkWriteable();

	MojRefCountedPtr<AsyncEmailWriter> m_writer;
	MojRefCountedPtr<AsyncOutputStream> m_sink;

	MojSignal<>::Slot<AsyncFlowControl>	m_sinkFullSlot;
	MojSignal<>::Slot<AsyncFlowControl>	m_sinkWriteableSlot;
};

#endif /* ASYNCFLOWCONTROL_H_ */
