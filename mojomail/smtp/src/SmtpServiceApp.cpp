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

#include "SmtpCommon.h"
#include "SmtpServiceApp.h"
#include "db/MojDbClient.h"
#include "db/MojDbServiceDefs.h"
#include "SmtpConfig.h"

int main(int argc, char** argv)
{
	SmtpServiceApp app;
	int mainResult = app.main(argc, argv);
	return mainResult;
}

const char* const SmtpServiceApp::ServiceName = "com.palm.smtp";

SmtpServiceApp::SmtpServiceApp()
: m_dbClient(&m_service),
  m_tempDbClient(&m_service, MojDbServiceDefs::TempServiceName)
{
}

MojErr SmtpServiceApp::open()
{
	MojErr err;
	
	// Call superclass open
	err = Base::open();
	MojErrCheck(err);
	
	// Initialize service
	err = m_service.open(ServiceName);
	MojErrCheck(err);
	
	err = m_service.attach(m_reactor.impl());
	MojErrCheck(err);
	
	m_handler.reset(new SmtpBusDispatcher(*this));
	MojAllocCheck(m_handler.get());
	
	err = m_handler->InitHandler();
	MojErrCheck(err);
	
	err = m_service.addCategory(MojLunaService::DefaultCategory, m_handler.get());
	MojErrCheck(err);
	
	fprintf(stderr, "Running!\n");

	return MojErrNone;
}

MojErr SmtpServiceApp::configure(const MojObject& conf)
{
	MojErr err;
	err = MojServiceApp::configure(conf);
	MojErrCheck(err);

	err = SmtpConfig::GetConfig().ParseConfig(conf);
	MojErrCheck(err);

	return MojErrNone;
}
