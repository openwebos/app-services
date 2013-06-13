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

#include "ImapServiceApp.h"
#include "ImapBusDispatcher.h"
#include "exceptions/ExceptionUtils.h"

int main(int argc, char** argv)
{
	ImapServiceApp app;

	// Set terminate handler
	std::set_terminate(&ExceptionUtils::TerminateHandler);

	int mainResult = app.main(argc, argv);

	return mainResult;
}

const char* const ImapServiceApp::ServiceName = "com.palm.imap";

ImapServiceApp::ImapServiceApp()
: m_dbClient(&m_service)
{
	//s_log.level(MojLogger::LevelTrace);
}

MojErr ImapServiceApp::open()
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
	
	m_handler.reset(new ImapBusDispatcher(*this));
	MojAllocCheck(m_handler.get());
	
	err = m_handler->InitHandler();
	MojErrCheck(err);
	
	err = m_service.addCategory(MojLunaService::DefaultCategory, m_handler.get());
	MojErrCheck(err);

	fprintf(stderr, "Running!\n");

	return MojErrNone;
}

MojErr ImapServiceApp::configure(const MojObject& conf)
{
	MojErr err;
	err = MojServiceApp::configure(conf);
	MojErrCheck(err);

	err = ImapConfig::GetConfig().ParseConfig(conf);
	MojErrCheck(err);

	return MojErrNone;
}
