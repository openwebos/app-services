// @@@LICENSE
//
//      Copyright (c) 2010-2012 Hewlett-Packard Development Company, L.P.
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

#ifndef SMTPSERVICEAPP_H_
#define SMTPSERVICEAPP_H_

#include <glib.h>
#include "core/MojReactorApp.h"
#include "core/MojGmainReactor.h"
#include "luna/MojLunaService.h"
#include "db/MojDbServiceClient.h"
#include "SmtpBusDispatcher.h"
#include "client/DbMergeProxy.h"

class SmtpServiceApp : public MojReactorApp<MojGmainReactor>
{
public:
	static const char* const ServiceName;
	
	SmtpServiceApp();
	virtual MojErr open();
	
	MojDbServiceClient &GetDbClient() { return m_dbClient; }
	MojDbServiceClient &GetTempDbClient() { return m_tempDbClient; }
	MojLunaService &GetService() { return m_service; }
	
	// Overrides MojApp::configure
	virtual MojErr configure(const MojObject& conf);

private:
	typedef MojReactorApp<MojGmainReactor> Base;
	
	MojLunaService m_service;
	DbMergeProxy m_dbClient;
	MojDbServiceClient m_tempDbClient;
	
	// Main category handler
	MojRefCountedPtr<SmtpBusDispatcher> m_handler;
};

#endif /*SMTPSERVICEAPP_H_*/
