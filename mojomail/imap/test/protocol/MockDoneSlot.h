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

#ifndef MOCKDONESLOT_H_
#define MOCKDONESLOT_H_

#include "core/MojSignal.h"

class MockDoneSlot;

class DoneSlotHandler : public MojSignalHandler
{
public:
	DoneSlotHandler(MockDoneSlot& owner)
	: m_owner(owner), m_doneSlot(this, &DoneSlotHandler::Done) {}
	virtual ~DoneSlotHandler() {}
	
	MojErr Done();
	
	MockDoneSlot& m_owner;
	MojSignal<>::Slot<DoneSlotHandler> m_doneSlot;
};

class MockDoneSlot
{
public:
	MockDoneSlot() : m_called(false), m_handler(new DoneSlotHandler(*this)) {}
	virtual ~MockDoneSlot() {}
	
	MojSignal<>::SlotRef GetSlot() { return m_handler->m_doneSlot; }
	
	virtual void Done() { m_called = true; }
	
	virtual bool Called() { return m_called; }
	
protected:
	bool m_called;
	MojRefCountedPtr<DoneSlotHandler> m_handler;
};

#endif /*MOCKDONESLOT_H_*/
