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

#ifndef EASBODYWRITER_H_
#define EASBODYWRITER_H_

#include <core/MojObject.h>
#include <core/MojSignal.h>
#include <boost/scoped_ptr.hpp>
#include "async/AsyncWriter.h"
#include "mimeparser/MimeEmailParser.h"
#include <boost/shared_ptr.hpp>
#include "data/CommonData.h"
#include "CommonErrors.h"

class FileCacheClient;
class FileCacheResizerOutputStream;
class AsyncIOChannel;
class AsyncIOChannelFactory;
class BaseOutputStream;
class PreviewTextExtractorOutputStream;

class AsyncEmailParser : public MojSignalHandler, protected ParseEventHandler
{
public:
	AsyncEmailParser();
	virtual ~AsyncEmailParser();

	// Used by EAS for saving bodies without any MIME processing.
	void EnableRawBodyMode(const std::string& mimeType);

	// Enable parsing email headers
	void EnableHeaderParsing();

	// Enable saving bodies to the filecache
	void EnableBodyParsing(FileCacheClient& fileCacheClient, MojInt64 estimatedSize);

	// Enable generating preview text
	void EnablePreviewText(size_t size);

	// Begin parsing an email
	virtual void Begin();

	// Finish parsing an email
	virtual void End();

	// Parse data. Returns the number of bytes handled.
	size_t ParseData(const char* data, size_t length, bool isEOF);

	// Parse a line. Returns false if the parser is paused (line not parsed).
	bool ParseLine(const char* data, size_t length);

	// Set slot that gets called when the parser is ready for more data
	void SetReadySlot(MojSignal<>::SlotRef readySlot) { m_readySignal.connect(readySlot); }
	void SetDoneSlot(MojSignal<>::SlotRef doneSlot) { m_doneSignal.connect(doneSlot); }

	// Returns true if the parser is willing to accept more data
	virtual bool IsReadyForData() const { return !m_paused; }

	// Returns true if we were unable to parse the email or write to the filecache
	virtual bool HasFatalError() const { return m_fatalError.errorCode != MailError::NONE; }

	// Set Email object to use for storing headers
	void SetEmail(const EmailPtr& email) { m_email = email; }

	// Returns the email headers
	const EmailPtr& GetEmail() { return m_email; }

	// Returns the parts parsed
	void GetFlattenedParts(EmailPartList& parts);

	// Returns a list of file cache paths created
	std::vector<std::string> GetPaths() const { return m_fileCachePaths; }

	std::string GetPreviewText(const EmailPartList& parts);

	// For testing/debugging only
	MimeEmailParser* GetMimeEmailParser() { return m_mimeParser.get(); }
	void SetChannelFactory(const boost::shared_ptr<AsyncIOChannelFactory>& factory) { m_channelFactory = factory; }

	void Status(MojObject& status) const;

protected:
	// Handle MimeParser events
	void HandleBeginEmail();
	void HandleEndEmail();

	void HandleBeginPart();
	void HandleEndPart();

	void HandleBeginHeaders();
	void HandleEndHeaders(bool complete);

	void HandleBeginBody();
	void HandleBodyData(const char* data, size_t length);
	void HandleEndBody(bool complete);

	void HandleContentType(const std::string& type, const std::string& subtype, const std::map<std::string, std::string>& headers);
	void HandleHeader(const std::string& fieldName, const std::string& line);

	// Other stuff
	virtual void Pause();
	virtual void Unpause();

	void FileCacheInsert();
	MojErr FileCacheInsertResponse(MojObject& response, MojErr err);
	MojErr ChannelClosed();

	void CheckDone();

	void CleanupStreams();

	EmailPart& CurrentPart();

	static MojLogger&	s_log;

	FileCacheClient*	m_fileCacheClient;

	bool		m_isMimeStream;
	bool		m_parseEmailHeaders;
	bool		m_parseEmailBodies;
	size_t		m_previewTextSize;
	MojInt64	m_estimatedSize;
	bool		m_endOfStream;
	bool		m_paused;
	bool		m_doneParsing;
	bool		m_done;

	MailError::ErrorInfo	m_fatalError;

	std::string	m_path;
	EmailPartPtr	m_partBeingWritten;

	boost::scoped_ptr<MimeEmailParser>	m_mimeParser;

	EmailPartPtr	m_rootPart; // represents the email
	EmailPartList	m_partStack; // stack of nested parts

	EmailPtr		m_email;

	std::vector<std::string>	m_fileCachePaths; // list of paths we've gotten from filecache

	MojRefCountedPtr<AsyncIOChannel>					m_channel;
	boost::shared_ptr<AsyncIOChannelFactory>			m_channelFactory;
	MojRefCountedPtr<BaseOutputStream>					m_outputStream;
	MojRefCountedPtr<FileCacheResizerOutputStream>		m_resizerOutputStream;
	MojRefCountedPtr<PreviewTextExtractorOutputStream>	m_previewTextExtractorOutputStream;

	MojSignal<>											m_readySignal;
	MojSignal<>											m_doneSignal;
	MojSignal<MojObject&, MojErr>::Slot<AsyncEmailParser>	m_fileCacheInsertSlot;
	MojSignal<>::Slot<AsyncEmailParser>						m_channelClosedSlot;
};

#endif /* EASBODYWRITER_H_ */
