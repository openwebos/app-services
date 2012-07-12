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

#ifndef FETCHRESPONSEPARSER_H_
#define FETCHRESPONSEPARSER_H_

#include "protocol/ImapResponseParser.h"
#include <boost/scoped_ptr.hpp>
#include <vector>
#include "data/CommonData.h"
#include "stream/BaseOutputStream.h"

class Rfc3501Tokenizer;
class ImapParser;
class SemanticActions;
class ImapEmail;

// Represents a fetch response, possibly a new email or a flag update or body part
struct FetchUpdate
{
	FetchUpdate(unsigned int msgNum, const boost::shared_ptr<ImapEmail> &email, bool flagsUpdated)
	: msgNum(msgNum), email(email), flagsUpdated(flagsUpdated) {}

	unsigned int msgNum;
	boost::shared_ptr<ImapEmail> email;

	bool flagsUpdated;
};

class FetchResponseParser : public ImapResponseParser
{
	typedef boost::shared_ptr<ImapEmail> EmailPtr;
public:
	FetchResponseParser(ImapSession& session);
	FetchResponseParser(ImapSession& session, DoneSignal::SlotRef doneSlot);
	virtual ~FetchResponseParser();
	
	void SetPartOutputStream(const OutputStreamPtr& outputStream);

	bool HandleUntaggedResponse(const std::string& line);
	bool HandleAdditionalData();
	
	const std::vector<FetchUpdate>&	GetUpdates() const { return m_emails; }

protected:
	// Parse the current token buffer. Returns true if it needs more data.
	bool Parse();
	void RequestMoreData();
	bool SkipRestOfResponse();

	boost::scoped_ptr<Rfc3501Tokenizer> m_tokenizer;
	boost::scoped_ptr<SemanticActions>	m_semantic;
	boost::scoped_ptr<ImapParser>		m_imapParser;

	OutputStreamPtr						m_partOutputStream;
	size_t								m_literalBytesRemaining;

	std::vector<FetchUpdate>			m_emails;

	bool								m_recoveringFromError;
};

#endif /*FETCHRESPONSEPARSER_H_*/
