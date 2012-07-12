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

#ifndef POPCOMMANDRESULT_H_
#define POPCOMMANDRESULT_H_

#include "core/MojSignal.h"
#include "PopDefs.h"
#include <boost/exception_ptr.hpp>

class PopCommandResult : public MojSignalHandler
{
public:
	PopCommandResult();
	PopCommandResult(MojSignal<>::SlotRef doneSlot);

	void ConnectDoneSlot(MojSignal<>::SlotRef doneSlot);

	void SetException(const std::exception& e);
	void SetException();

	void CheckException();
	void Done();

protected:
	virtual ~PopCommandResult();

	MojSignal<>				m_doneSignal;
	boost::exception_ptr	m_exception;
};

#endif /* POPCOMMANDRESULT_H_ */
