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

#ifndef APPENDCOMMAND_H_
#define APPENDCOMMAND_H_

#include "commands/ImapSyncSessionCommand.h"
#include "db/MojDbClient.h"
#include "protocol/AppendResponseParser.h"
#include "data/Email.h"
#include "email/AsyncEmailWriter.h"
#include "stream/CounterOutputStream.h"

class StoreResponseParser;

class AppendCommand : public ImapSyncSessionCommand
{
	typedef boost::shared_ptr<Email> EmailPtr;

public:
	AppendCommand(ImapSession& session, const MojObject& folderId, const EmailPtr& emailPtr, const std::string& flags, const std::string& folderName);
	virtual ~AppendCommand();

	void RunImpl();

	UID GetUid() { return m_uid; }
	UID GetUidValidity() { return m_uidValidity; }

protected:

	static const char* const APPEND_TERMINATION_STRING;

	void CalculateEmailSize();
	MojErr FinishCalculateEmailSize(const std::exception* exc);

	void SendAppendRequest();
	MojErr HandleAppendContinueResponse();

	void WriteEmail();
	MojErr FinishWriteEmail(const std::exception* exc);
	MojErr HandleAppendDoneResponse();

	static const int APPEND_WRITE_COMPLETE_TIMEOUT;
	static const int APPEND_RESPONSE_TIMEOUT;

	MojRefCountedPtr<AppendResponseParser> 		m_appendToServerResponseParser;
	MojRefCountedPtr<AsyncEmailWriter>			m_emailWriter; // signal handlers must be refcounted
	MojRefCountedPtr<CounterOutputStream>		m_counter;
	size_t										m_bytesLeft;

	EmailPtr				m_emailP;
	std::string				m_flags;
	std::string				m_folderName;
	UID 					m_uid;
	UID 					m_uidValidity;

	AsyncEmailWriter::EmailWrittenSignal::Slot<AppendCommand> 	m_calculateDoneSlot;
	ImapResponseParser::ContinuationSignal::Slot<AppendCommand>	m_appendToServerContinueSlot;
	AsyncEmailWriter::EmailWrittenSignal::Slot<AppendCommand> 	m_writeDoneSlot;
	ImapResponseParser::DoneSignal::Slot<AppendCommand>			m_appendToServerDoneSlot;
};

#endif /* APPENDCOMMAND_H_ */
