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

#ifndef PARSEEMLHANDLER_H_
#define PARSEEMLHANDLER_H_

#include "data/Email.h"
#include "client/Command.h"
#include "stream/LineReader.h"
#include "core/MojServiceMessage.h"
#include "PopBusDispatcher.h"

#include <vector>

class PopEmail;
class AsyncEmailParser;

class ParseEmlHandler : public Command
{
public:
	static const int READ_TIMEOUT_IN_SECONDS;

	ParseEmlHandler(PopBusDispatcher* busDispatcher, boost::shared_ptr<BusClient> bClient, MojRefCountedPtr<CommandManager> manager, MojRefCountedPtr<MojServiceMessage> msg, std::string filePath);
	virtual ~ParseEmlHandler();

	void Run();
	void Cancel();

protected:
	static MojLogger& s_log;

private:
	void 	RunImpl();
	void 	CancelImpl();
	bool 	ParseEml(std::string buf, bool isEof = false);
	void 	ParseEmlFailed(const std::exception &e);
	void 	ParseEmlDone(bool isError = false);
	MojErr 	ParserReady();
	MojErr 	ParserDone();
	MojErr	HandleEmlDataAvailable();
	bool 	ReadEmlAndParseData();
	void 	GetEmlFile();
	void 	Complete();

	PopBusDispatcher*					m_popBusDispatcher;
	boost::shared_ptr<BusClient>		m_bClient;
	boost::shared_ptr<FileCacheClient>	m_fileCacheClient;
	boost::shared_ptr<Email>			m_email;
	MojRefCountedPtr<MojServiceMessage>	m_msg;
	std::string							m_filePath;
	bool								m_paused;
	bool								m_needMoreData;
	// Used to construct AsyncIOChannel objects for files
	MojRefCountedPtr<AsyncIOChannel>	m_emlChannel;			// must use MojRefCountedPtr
	LineReaderPtr   					m_lineReader;

	MojRefCountedPtr<AsyncEmailParser>	m_emailParser;

	boost::shared_ptr<AsyncIOChannelFactory>	m_ioFactory;
	InputStreamPtr								m_inputStream;

	AsyncIOChannel::ReadableSignal::Slot<ParseEmlHandler>		m_emlChannelReadableSlot;
	MojSignal<>::Slot<ParseEmlHandler>						m_parserReadySlot;
	MojSignal<>::Slot<ParseEmlHandler>						m_parserDoneSlot;
};

#endif /* PARSEEMLHANDLER_H_ */
