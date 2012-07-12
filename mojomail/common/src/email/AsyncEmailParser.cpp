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

#include "email/AsyncEmailParser.h"
#include "client/FileCacheClient.h"
#include "async/FileCacheResizerOutputStream.h"
#include "async/GIOChannelWrapper.h"
#include "CommonMacros.h"
#include "stream/BaseOutputStream.h"
#include "stream/UTF8DecoderOutputStream.h"
#include "stream/QuotePrintableDecoderOutputStream.h"
#include "stream/Base64DecoderOutputStream.h"
#include "stream/PreviewTextExtractorOutputStream.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/foreach.hpp>
#include "data/EmailPart.h"
#include "util/LogUtils.h"
#include "data/Email.h"
#include "email/Rfc2047Decoder.h"
#include "mimeparser/EmailHeaderFieldParser.h"
#include "email/PreviewTextGenerator.h"

using namespace std;

MojLogger& AsyncEmailParser::s_log = LogUtils::s_commonLog;

AsyncEmailParser::AsyncEmailParser()
: m_fileCacheClient(NULL),
  m_isMimeStream(true),
  m_parseEmailHeaders(false),
  m_parseEmailBodies(false),
  m_previewTextSize(0),
  m_estimatedSize(0),
  m_paused(false),
  m_doneParsing(false),
  m_done(false),
  m_channelFactory(new GIOChannelWrapperFactory()),
  m_readySignal(this),
  m_doneSignal(this),
  m_fileCacheInsertSlot(this, &AsyncEmailParser::FileCacheInsertResponse),
  m_channelClosedSlot(this, &AsyncEmailParser::ChannelClosed)
{
}

AsyncEmailParser::~AsyncEmailParser()
{
}

void AsyncEmailParser::EnableRawBodyMode(const std::string& mimeType)
{
	m_isMimeStream = false;

	// Setup body part
	m_rootPart.reset(new EmailPart(EmailPart::BODY));
	m_rootPart->SetMimeType(mimeType);
	m_partStack.push_back(m_rootPart);
}

void AsyncEmailParser::EnableHeaderParsing()
{
	m_parseEmailHeaders = true;

	if (!m_email.get()) {
		m_email.reset(new Email());
	}
}

void AsyncEmailParser::EnableBodyParsing(FileCacheClient& fileCacheClient, MojInt64 estimatedSize)
{
	m_fileCacheClient = &fileCacheClient;
	m_parseEmailBodies = true;
	m_estimatedSize = estimatedSize;
}

void AsyncEmailParser::EnablePreviewText(size_t previewTextSize)
{
	m_previewTextSize = previewTextSize;
}

void AsyncEmailParser::Begin()
{
	if (m_isMimeStream) {
		m_mimeParser.reset(new MimeEmailParser(*this));
		m_mimeParser->BeginEmail();
	} else {
		HandleBeginBody();
	}
}

void AsyncEmailParser::End()
{
	if (m_isMimeStream) {
		m_mimeParser->EndEmail();
	} else {
		m_doneParsing = true;
		HandleEndBody(false);
	}

	if (!m_paused) {
		CheckDone();
	}
}

size_t AsyncEmailParser::ParseData(const char* data, size_t length, bool isEOF)
{
	if (m_paused) {
		return 0;
	}

	if (m_isMimeStream) {
		return m_mimeParser->ParseData(data, length, isEOF);
	} else {
		HandleBodyData(data, length);
		return length;
	}
}

bool AsyncEmailParser::ParseLine(const char* data, size_t length)
{
	if (m_paused) {
		return false;
	}

	if (m_isMimeStream) {
		m_mimeParser->ParseLine(data, length);
	} else {
		HandleBodyData(data, length);
	}

	return true;
}

void AsyncEmailParser::Pause()
{
	if (!m_paused) {
		m_paused = true;

		// Pause parser -- this causes it to return from parsing a block of data
		if (m_mimeParser.get()) {
			m_mimeParser->PauseParseData();
		}
	}
}

void AsyncEmailParser::Unpause()
{
	if (m_paused) {
		m_paused = false;

		m_readySignal.call();
	}
}

void AsyncEmailParser::HandleBeginEmail()
{
}

inline EmailPart& AsyncEmailParser::CurrentPart()
{
	return *(m_partStack.back());
}

void AsyncEmailParser::HandleBeginPart()
{
	EmailPartPtr part(new EmailPart(EmailPart::INLINE));

	if (!m_rootPart.get()) {
		// This part represents the email
		m_rootPart = part;
	} else {
		// Add as subpart of current part
		if (!m_partStack.empty()) {
			CurrentPart().AddSubpart(part);
		}
	}

	// Current part
	m_partStack.push_back(part);
}

void AsyncEmailParser::HandleBeginHeaders()
{
}

void AsyncEmailParser::HandleContentType(const std::string& type, const std::string& subtype, const std::map<std::string, std::string>& parameters)
{
	CurrentPart().SetMimeType(type + "/" + subtype);

	EmailHeaderFieldParser::ParameterMap::const_iterator it;

	it = parameters.find("charset");
	if (it != parameters.end()) {
		// Save charset
		CurrentPart().SetCharset(it->second);
	}

	it = parameters.find("name");
	if (it != parameters.end()) {
		// Save filename
		CurrentPart().SetDisplayName( Rfc2047Decoder::SafeDecodeText( it->second ) );
	}
}

void AsyncEmailParser::HandleHeader(const std::string& origFieldName, const std::string& fieldValue)
{
	EmailHeaderFieldParser headerParser;
	std::string fieldNameLower = boost::to_lower_copy(origFieldName);

	try {
		if (m_parseEmailBodies && headerParser.ParsePartHeaderField(CurrentPart(), fieldNameLower, fieldValue)) {
			// found a part header and parsed it into the part
		} else if (m_parseEmailHeaders && m_partStack.size() == 1 && m_email.get()) {
			// only try to parse email fields for the email headers, not other parts
			headerParser.ParseEmailHeaderField(*m_email, fieldNameLower, fieldValue);
		}
	} catch (const exception& e) {
		MojLogError(s_log, "error parsing header line: %s: %s", origFieldName.c_str(), fieldValue.c_str());
	}
}

void AsyncEmailParser::HandleEndHeaders(bool incompleteHeaders)
{
	// If no content-type was specified, assume it's text/plain
	if (CurrentPart().GetMimeType().empty()) {
		CurrentPart().SetMimeType("text/plain");
	}
}

void AsyncEmailParser::HandleEndPart()
{
	if (!m_partStack.empty()) {
		m_partStack.pop_back();
	}
}

void AsyncEmailParser::HandleBeginBody()
{
	// Create filecache entry
	if (m_fileCacheClient) {
		m_partBeingWritten = m_partStack.back();

		// Pause until the filecache entry is ready
		Pause();
		FileCacheInsert();
	} else {
		// No FileCache client
		m_outputStream.reset(NULL);
	}
}

void AsyncEmailParser::FileCacheInsert()
{
	bool isAttachment = CurrentPart().IsAttachment();
	string name;

	if (isAttachment) {
		name = m_partBeingWritten->GetDisplayName();
		if (name.empty()) name = "attachment" + m_partBeingWritten->GuessFileExtension();
	} else {
		name = "body" + m_partBeingWritten->GuessFileExtension();
	}

	m_fileCacheClient->InsertCacheObject(m_fileCacheInsertSlot, isAttachment ? "attachment" : "email", name.c_str(), m_estimatedSize, -1, -1);
}

MojErr AsyncEmailParser::FileCacheInsertResponse(MojObject& response, MojErr err)
{
	try {
		MojString path;
		err = response.getRequired("pathName", path);
		ErrorToException(err);

		m_path.assign(path.data());

		// Save to list of file cache paths so someone can clean them up later
		m_fileCachePaths.push_back(m_path);

		MojLogDebug(s_log, "preparing to write %s to '%s' with encoding '%s' charset '%s'", CurrentPart().GetMimeType().c_str(), m_path.c_str(), CurrentPart().GetEncoding().c_str(), CurrentPart().GetCharset().c_str());

		m_channel = m_channelFactory->OpenFile(m_path.c_str(), "w");
		m_channel->WatchClosed(m_channelClosedSlot);

		m_outputStream = m_channel->GetOutputStream();

		// Wrap output stream as needed
		m_resizerOutputStream.reset(new FileCacheResizerOutputStream(m_outputStream, *m_fileCacheClient, std::string(path.data()), m_estimatedSize));
		m_outputStream = m_resizerOutputStream;

		// Extract preview text
		if (m_previewTextSize > 0) {
			if (!CurrentPart().IsAttachment()) {
				m_previewTextExtractorOutputStream.reset(new PreviewTextExtractorOutputStream(m_outputStream, 8192));
				m_outputStream = m_previewTextExtractorOutputStream;
			}
		}

		// Decode charset
		string charset = CurrentPart().GetCharset();
		if (!charset.empty()) {
			try {
				m_outputStream.reset(new UTF8DecoderOutputStream(m_outputStream, charset.c_str()));
			} catch (const std::exception& e) {
				MojLogError(s_log, "error creating charset decoder stream: %s", e.what());

				// Fall back to ASCII
				m_outputStream.reset(new UTF8DecoderOutputStream(m_outputStream, "us-ascii"));
			}
		}

		// Decode encoding
		string encoding = CurrentPart().GetEncoding();
		if (boost::iequals(encoding, "base64")) {
			m_outputStream.reset(new Base64DecoderOutputStream(m_outputStream));
		} else if (boost::iequals(encoding, "quoted-printable")) {
			m_outputStream.reset(new QuotePrintableDecoderOutputStream(m_outputStream));
		}


	} catch (const std::exception& e) {
		MojLogError(s_log, "error creating file cache entry: %s", e.what());

		m_fatalError = MailError::ErrorInfo(MailError::INTERNAL_ERROR, e.what());
		m_path.clear();

		CleanupStreams();
	}

	// Resume -- should get called with HandleBodyData shortly
	Unpause();

	return MojErrNone;
}

void AsyncEmailParser::HandleBodyData(const char* data, size_t length)
{
	if (m_outputStream.get()) {
		//fprintf(stderr, "body line: {%s}\n", std::string(data, length).c_str());
		m_outputStream->Write(data, length);
	}
}

void AsyncEmailParser::HandleEndBody(bool incompleteBody)
{
	if (incompleteBody) {
		MojLogInfo(s_log, "part body incomplete");
	}

	// Need to wait for the channel to finish writing to disk
	if (m_channel.get()) {
		Pause();
	}

	// Close the file
	if (m_outputStream.get()) {
		m_outputStream->Flush();
		m_outputStream->Close();
	}

	if (!incompleteBody) {
		// Save path while we still have the part
		CurrentPart().SetLocalFilePath(m_path);
	}
}

MojErr AsyncEmailParser::ChannelClosed()
{
	MojLogDebug(s_log, "done writing part '%s'", m_partBeingWritten.get() ? m_partBeingWritten->GetLocalFilePath().c_str() : "");

	// Check for errors
	// FIXME this code is disabled until we can make binary-incompatible changes
	if (m_outputStream.get()) {
		MailError::ErrorInfo error = m_outputStream->GetError();

		if (error.errorCode != MailError::NONE) {
			m_fatalError = error;
			MojLogError(s_log, "error writing part to file: %s", error.errorText.c_str());

			if (m_partBeingWritten.get()) {
				// Clear path since we didn't finish writing
				m_partBeingWritten->SetLocalFilePath("");
			}
		}
	}

	// Extract preview text
	if (m_previewTextExtractorOutputStream.get() && m_partBeingWritten.get()) {
		string previewText = m_previewTextExtractorOutputStream->GetPreviewText();
		string mimeType = m_partBeingWritten->GetMimeType();

		previewText = PreviewTextGenerator::GeneratePreviewText(previewText, m_previewTextSize, boost::iequals(mimeType, "text/html"));
		m_partBeingWritten->SetPreviewText(previewText);
	}

	m_fileCacheInsertSlot.cancel();
	m_partBeingWritten.reset();

	CleanupStreams();

	if (m_doneParsing) {
		CheckDone();
	} else {
		// Unpause so we can continue parsing
		Unpause();
	}

	return MojErrNone;
}

void AsyncEmailParser::HandleEndEmail()
{
	// done parsing email ... may still be writing to disk though
	m_doneParsing = true;

	CheckDone();
}

void AsyncEmailParser::CheckDone()
{
	//fprintf(stderr, "checking if done: doneParsing=%d, partBeingWritten=%p\n", m_doneParsing, m_partBeingWritten.get());

	// Check if we reached the end of the email AND everything's written to disk
	if (m_doneParsing && m_partBeingWritten.get() == NULL && !m_done) {
		MojLogDebug(s_log, "done parsing email");
		m_done = true;
		m_doneSignal.fire();
	}
}

void AsyncEmailParser::CleanupStreams()
{
	m_outputStream.reset();
	m_resizerOutputStream.reset();
	m_previewTextExtractorOutputStream.reset();
}

void AsyncEmailParser::GetFlattenedParts(EmailPartList& parts)
{
	if (m_rootPart.get()) {
		EmailPart::GetFlattenedParts(m_rootPart, parts);
	}
}

string AsyncEmailParser::GetPreviewText(const EmailPartList& parts)
{
	BOOST_FOREACH(const EmailPartPtr& part, parts) {
		if (part->GetType() == EmailPart::BODY) {
			return part->GetPreviewText();
		}
	}

	return "";
}

void AsyncEmailParser::Status(MojObject& status) const
{
	MojErr err;

	err = status.put("paused", m_paused);
	ErrorToException(err);

	if (m_partBeingWritten.get()) {
		MojObject partStatus;
		err = partStatus.putString("path", m_path.c_str());
		ErrorToException(err);

		err = status.put("currentPart", partStatus);
		ErrorToException(err);
	}
}
