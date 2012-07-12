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

#ifndef POPSERVICEAPP_H_
#define POPSERVICEAPP_H_

#include "glibcurl/glibcurl.h"
#include "core/MojGmainReactor.h"
#include "core/MojReactorApp.h"
#include "db/MojDbServiceClient.h"
#include "luna/MojLunaService.h"
#include "db/MojDbServiceClient.h"
#include "PopBusDispatcher.h"
#include "client/DbMergeProxy.h"

class PopMain : public MojReactorApp<MojGmainReactor>
{
public:
	static const char* const ServiceName;

	PopMain();
	virtual MojErr open();
	virtual MojErr close();

	//MojLunaService& GetService();

	static void Shutdown();

private:
	typedef MojReactorApp<MojGmainReactor> Base;

	virtual MojErr handleArgs(const StringVec& args);
	virtual MojErr displayUsage();

	static PopMain* 					s_instance;
	MojRefCountedPtr<PopBusDispatcher> 	m_handler;
	DbMergeProxy						m_dbClient;
	MojLunaService  					m_service;
};

#endif /* POPSERVICEAPP_H_ */
