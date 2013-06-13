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

#include "PopConfig.h"
#include "PopMain.h"

using namespace std;

const char* const PopMain::ServiceName = "com.palm.pop";
PopMain* PopMain::s_instance = NULL;

int main(int argc, char** argv)
{
	glibcurl_init();
	PopMain app;
	int mainResult = app.main(argc, argv);
	glibcurl_cleanup();

	return mainResult;
}

PopMain::PopMain()
: m_dbClient(&m_service)
{
	if (s_instance == NULL)
		s_instance = this;
}

MojErr PopMain::open()
{
	MojLogNotice(s_log, _T("%s starting..."), name().data());

	try {
		MojErr err = Base::open();
		MojErrCheck(err);

		// open service
		err = m_service.open(ServiceName);
		MojErrCheck(err);

		err = m_service.attach(m_reactor.impl());
		MojErrCheck(err);

		// create and attach service handler
		m_handler.reset(new PopBusDispatcher(*this, m_dbClient, m_service));
		MojAllocCheck(m_handler.get());
		err = m_handler->Init();
		MojErrCheck(err);

		err = m_service.addCategory(MojLunaService::DefaultCategory, m_handler.get());
		MojErrCheck(err);

		fprintf(stderr, "Running!\n");
	} catch (const std::exception& e) {
		//TODO
	}

	MojLogNotice(s_log, _T("%s started."), name().data());

	return MojErrNone;
}

MojErr PopMain::close()
{
	MojLogNotice(s_log, _T("%s stopping..."), name().data());

	MojErr err = MojErrNone;
	MojErr errClose = m_service.close();
	MojErrAccumulate(err, errClose);
	errClose = Base::close();
	MojErrAccumulate(err, errClose);

	MojLogNotice(s_log, _T("%s stopped."), name().data());

	return err;
}

void PopMain::Shutdown()
{
	s_instance->shutdown();
}

MojErr PopMain::handleArgs(const StringVec& args)
{
	MojErr err = Base::handleArgs(args);
	MojErrCheck(err);

	return MojErrNone;
}

MojErr PopMain::displayUsage()
{
	MojErr err = displayMessage("Usage: mojomail-pop\n");
	MojErrCheck(err);

	return MojErrNone;
}
