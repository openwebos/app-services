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

#include "protocol/UntaggedUpdateParser.h"
#include "client/FolderSession.h"
#include "client/ImapSession.h"
#include "commands/HandleUpdateCommand.h"
#include "commands/ImapCommandResult.h"
#include "data/EmailAdapter.h"
#include "data/ImapEmail.h"
#include "parser/Rfc3501Tokenizer.h"
#include "sync/UIDMap.h"
#include "ImapPrivate.h"

UntaggedUpdateParser::UntaggedUpdateParser(ImapSession& session)
: ImapResponseParser(session),
  m_updateCommandSlot(this, &UntaggedUpdateParser::UpdateCommandResponse)
{
}

UntaggedUpdateParser::~UntaggedUpdateParser()
{
}

bool UntaggedUpdateParser::HandleUntaggedResponse(const string& line)
{
	Rfc3501Tokenizer t(line);

	long number;
	if(t.next() == TK_TEXT && t.numberValue(number)) {
		if(t.next() != TK_SP) {
			ThrowParseException("expected space after number");
		}

		if(t.next() != TK_TEXT) {
			ThrowParseException("expected text after number and space");
		}

		string key = t.valueUpper();

		const boost::shared_ptr<FolderSession>& folderSession = m_session.GetFolderSession();

		if(folderSession.get() && (key == "EXISTS" || key == "EXPUNGE" || key == "FETCH")) {
			if(key == "EXISTS") {
				int oldCount = folderSession->GetMessageCount();
				folderSession->SetMessageCount(number);

				if(number < oldCount) {
					/* Per RFC 3501 5.2:
					 * "In particular, it is NOT permitted to send an EXISTS response that would reduce the
					 * number of messages in the mailbox; only the EXPUNGE response can do this."
					 */
					MojLogWarning(m_log, "EXISTS count less than expected; possible server bug or desync");
				}

				if(folderSession->HasUIDMap()) {
					const boost::shared_ptr<UIDMap>& uidMap = folderSession->GetUIDMap();
					uidMap->UpdateMsgCount(number);

					if(number > oldCount) {
						// new messages
						MojLogInfo(m_log, "EXISTS update: %ld new messages on server", number - oldCount);
						m_session.FetchNewHeaders(folderSession->GetFolderId());
					}
				}
			} else if(key == "EXPUNGE") {
				// decrement message count
				int oldCount = folderSession->GetMessageCount();
				folderSession->SetMessageCount(oldCount - 1);

				if(folderSession->HasUIDMap()) {
					const boost::shared_ptr<UIDMap>& uidMap = folderSession->GetUIDMap();
					UID uid = uidMap->Remove(number);
					MojLogInfo(m_log, "UID %d (msg %ld) expunged from server", uid, number);

					MojObject newFlags(MojObject::Undefined);
					m_handleUpdateCommand.reset(new HandleUpdateCommand(m_session, folderSession->GetFolderId(), uid, true, newFlags));

					// Set up result so we get notified when it's done
					m_result = m_handleUpdateCommand->GetResult();
					m_result->ConnectDoneSlot(m_updateCommandSlot);

					m_handleUpdateCommand->Run();
				}
			} else if(key == "FETCH") {
				m_fetchParser.reset(new FetchResponseParser(m_session));

				// TODO handle multi-line fetch responses
				bool done = m_fetchParser->HandleUntaggedResponse(line);

				if(done) {
					BOOST_FOREACH(const FetchUpdate& update, m_fetchParser->GetUpdates()) {
						if(folderSession->HasUIDMap()) {
							const boost::shared_ptr<UIDMap>& uidMap = folderSession->GetUIDMap();
							UID uid = uidMap->GetUID(update.msgNum);
							if(uid > 0 && update.flagsUpdated) {
								MojLogInfo(m_log, "UID %d (msg %ld) flags updated", uid, number);

								MojObject newFlags;
								EmailAdapter::SerializeFlags(*update.email, newFlags);

								m_handleUpdateCommand.reset(new HandleUpdateCommand(m_session, folderSession->GetFolderId(), uid, update.email->IsDeleted(), newFlags));

								// Set up result so we get notified when it's done
								m_result = m_handleUpdateCommand->GetResult();
								m_result->ConnectDoneSlot(m_updateCommandSlot);

								m_handleUpdateCommand->Run();

								// only handle one update
								break;
							}
						}
					}
				}
			}
		}
	} else if(boost::iequals(t.value(), "BYE")) {
		MojLogInfo(m_log, "received BYE from server: %s", line.c_str());
	}

	return true;
}

MojErr UntaggedUpdateParser::UpdateCommandResponse()
{
	try {
		m_result->CheckException();
	} catch(const exception& e) {
		// FIXME
	}

	m_session.ParserFinished(this);

	return MojErrNone;
}
