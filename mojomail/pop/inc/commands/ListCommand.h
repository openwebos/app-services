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

#ifndef LISTCOMMAND_H_
#define LISTCOMMAND_H_

#include "commands/PopMultiLineResponseCommand.h"
#include "data/UidMap.h"

class ListCommand : public PopMultiLineResponseCommand
{
public:
	static const char* const	COMMAND_STRING;

	ListCommand(PopSession& session, boost::shared_ptr<UidMap>& uidMapPtr);
	virtual ~ListCommand();

	virtual void RunImpl();
private:
	virtual MojErr HandleResponse(const std::string& line);

	boost::shared_ptr<UidMap>	m_uidMapPtr;
};

#endif /* LISTCOMMAND_H_ */
