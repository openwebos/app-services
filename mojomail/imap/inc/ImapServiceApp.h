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

#ifndef IMAPSERVICEAPP_H_
#define IMAPSERVICEAPP_H_

#include <glib.h>
#include "core/MojReactorApp.h"
#include "core/MojGmainReactor.h"
#include "luna/MojLunaService.h"
#include "db/MojDbServiceClient.h"
#include "ImapBusDispatcher.h"
#include "ImapConfig.h"
#include "client/DbMergeProxy.h"

class ImapServiceApp : public MojReactorApp<MojGmainReactor>
{
public:
	static const char* const ServiceName;
	
	ImapServiceApp();

	// Overrides MojApp::open
	virtual MojErr open();
	
	// Overrides MojApp::configure
	virtual MojErr configure(const MojObject& conf);

	MojDbServiceClient&		GetDbClient()	{ return m_dbClient; }
	MojLunaService& 		GetService()	{ return m_service; }
	
	ImapConfig& GetConfig() { return ImapConfig::GetConfig(); }

private:
	typedef MojReactorApp<MojGmainReactor> Base;
	
	MojLunaService		m_service;
	//MojDbServiceClient	m_dbClient;
	DbMergeProxy		m_dbClient;

	// Main category handler
	MojRefCountedPtr<ImapBusDispatcher> m_handler;
};

#endif /*IMAPSERVICEAPP_H_*/
