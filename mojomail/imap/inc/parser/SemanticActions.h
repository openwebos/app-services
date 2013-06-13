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

#ifndef SEMANTICACTIONS_H_
#define SEMANTICACTIONS_H_

#include "data/CommonData.h"
#include "data/Email.h"
#include "stream/BaseOutputStream.h"
#include "exceptions/Rfc3501ParseException.h"

#include <vector>
#include <string>
#include <stack>
#include <boost/scoped_ptr.hpp>

class ImapEmail;
class Rfc3501Tokenizer;
class ByteBufferOutputStream;

enum StringType {
	ST_Null,
	ST_Quoted,
	ST_Literal
};


class SemanticActions {
public:
	SemanticActions(Rfc3501Tokenizer& tokenizer);
	virtual ~SemanticActions();
	
	unsigned int GetMsgNum() const { return m_msgNum; }
	const boost::shared_ptr<ImapEmail> GetEmail() const { return m_emailStack.top().m_email; }
	bool GetFlagsUpdated() const { return m_flagsUpdated; }

	bool	ExpectingBinaryData() const { return m_expectBinaryData; }
	size_t	ExpectedDataLength() const { return m_binaryDataLength; }

	void SetBodyOutputStream(const OutputStreamPtr& os) { m_bodyOutputStream = os; }
	const OutputStreamPtr&	GetCurrentOutputStream() const { return m_currentOutputStream; }

	// These are the semantic actions that the parser calls
	void endFetch(void);
	void expectBodyData(void);
	void handleBodyData(void);
	void readAheadLiteral(void);
	void endLiteralBytes(void);
	void saveQuotedString(void);
	void saveNullString(void);
	void saveLiteralString(void);
	void saveAtomString(void);
	void saveInternalDate(void);
	void saveNonZeroNumber(void);
	void saveUID(void);
	void saveMsgNum(void);

	void zeroPartWorkaround();

	void setSection();
	void setHeadersSection();

	void beginNestedEmail(void);
	void endNestedEmail(void);

	// BODYSTRUCTURE parsing
	void beginBodyStructure(void);
	void endBodyStructure(void);

	void beginPartSet(void);
	void endPartSet(void);
	void beginPartList(void);
	void endPartList(void);

	void setContentEncoding(void);
	void setPartSize(void);
	void setMimeType(void);
	void setMimeSubtype(void);
	void setMimeTypeMessage(void);
	void setMimeTypeText(void);
	void setMimeSubtypeRfc822(void);

	void beginFlags(void);
	void keywordFlagAtom(void);
	void systemFlagAtom(void);

	void setPartDisposition();
	void setPartParameterName();
	void setPartParameterValue();
	void setContentId();

	// helper functions
	std::string locationString(void);
	std::string nullableString();

	// Envelope
	void envSubject();

	void envSenderAddressList();
	void envFromAddressList();
	void envToAddressList();
	void envReplyToAddressList();
	void envBccAddressList();
	void envCcAddressList();

	void envBeginAddressList();

	void envDate();
	void envInReplyTo();
	void envMessageId();

	void envEndAddress();
	void envAddrName();
	void envAddrAdl();
	void envAddrMailbox();
	void envAddrHost();

	void saveMediaSubtype();
	
protected:
	// Temporary envelope values
	struct Envelope
	{
		std::string	m_addressDisplayName;
		std::string m_addressEmail;
		EmailAddressListPtr m_addressList;

		const EmailAddressListPtr& GetCurrentAddressList()
		{
			assert(m_addressList.get());

			return m_addressList;
		}
	} m_envelope;

	struct ImapStructurePart;
	typedef boost::shared_ptr<ImapStructurePart> ImapStructurePartPtr;

	struct ImapStructurePart
	{
		std::string partSection;

		std::string mimeType;
		std::string mimeSubtype;

		std::string charset;
		std::string encoding;

		std::string name;

		std::string contentId;
		std::string disposition;

		int size;

		std::vector<ImapStructurePartPtr> subparts;
	};

	struct NestedEmail
	{
		boost::shared_ptr<ImapEmail>	m_email;

		// Stack of multipart container parts
		std::stack< boost::shared_ptr<ImapStructurePart> >	m_multipartStack;
		boost::shared_ptr<ImapStructurePart>				m_currentPart;
	};

	NestedEmail& GetNestedEmail()
	{
		return m_emailStack.top();
	}

	std::stack< boost::shared_ptr<ImapStructurePart> >& GetPartStack()
	{
		return GetNestedEmail().m_multipartStack;
	}

	ImapEmail& GetCurrentEmail()
	{
		return *(GetNestedEmail().m_email);
	}

	const inline ImapStructurePartPtr& GetCurrentPart()
	{
		if( unlikely(m_currentPart.get() == NULL) ) {
			assert(m_currentPart.get());
			throw Rfc3501ParseException("no current part", __FILE__, __LINE__);
		}

		return m_currentPart;
	}

	// Create a part and add it as a subpart of the current multipart container
	void CreatePart();

	// Convert ImapStructurePart into EmailPart objects in the ImapEmail
	void SerializeParts(EmailPartList& emailParts, const ImapStructurePartPtr& imapPart, int depth = 0);

	std::string GetCurrentString(bool lowercase) const;
	static std::string ParsePhrase(const std::string& phrase);
	static std::string ParseText(const std::string& phrase);

	void ParseExtraHeader(const std::string& headerName, const std::string& value);
	void ParseExtraHeaders(const std::string& headerString);

	Rfc3501Tokenizer& m_tokenizer;

	// Temporary parser values
	StringType	m_stringType;
	std::string	m_stringValue;
	MojUInt32	m_nzNumber;

	// BODYSTRUCTURE temporary data
	std::string m_parameterName;

	unsigned int							m_msgNum;

	std::stack<NestedEmail>					m_emailStack;
	boost::shared_ptr<ImapStructurePart>	m_currentPart;


	bool m_expectBinaryData;
	int m_binaryDataLength;

	bool m_flagsUpdated;

	enum SectionType
	{
		Section_None,
		Section_Body,
		Section_Headers
	};

	SectionType m_sectionType;

	OutputStreamPtr m_currentOutputStream;
	OutputStreamPtr m_bodyOutputStream;
	MojRefCountedPtr<ByteBufferOutputStream> m_bufferOutputStream;
};

#endif /*SEMANTICACTIONS_H_*/
