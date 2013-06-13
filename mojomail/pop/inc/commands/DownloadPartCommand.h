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

#ifndef DOWNLOADPARTCOMMAND_H_
#define DOWNLOADPARTCOMMAND_H_

#include "PopClient.h"
#include "client/DownloadListener.h"
#include "commands/PopClientCommand.h"
#include "core/MojObject.h"

class DownloadPartCommand : public PopClientCommand
{
public:
	DownloadPartCommand(PopClient& client, const MojObject& folderId, const MojObject& emailId, const MojObject& partId, boost::shared_ptr<DownloadListener>& listener);
	~DownloadPartCommand();

	virtual void RunImpl();
private:
	MojObject							m_folderId;
	MojObject							m_emailId;
	MojObject 							m_partId;
	boost::shared_ptr<DownloadListener>	m_listener;
};

#endif /* DOWNLOADPARTCOMMAND_H_ */
