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

#include "email/AsyncEmailWriter.h"
#include "async/GIOChannelWrapper.h"
#include "data/CommonData.h"
#include "data/Email.h"
#include "data/EmailPart.h"
#include "util/StringUtils.h"
#include "sandbox.h"
#include "exceptions/MailException.h"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/foreach.hpp>
#include "CommonPrivate.h"

using namespace std;

#define CATCH_AS_EMAIL_FAILED \
	catch(const std::exception& e) { WriteEmailFailed(e); } \
	catch(...) { WriteEmailFailed(boost::unknown_exception()); }

#define CATCH_AS_PART_FAILED \
	catch(const std::exception& e) { WritePartFailed(e); } \
	catch(...) { WritePartFailed(boost::unknown_exception()); }

MojLogger& AsyncEmailWriter::s_log = LogUtils::s_commonLog;
MojLogger& FilePartWriter::s_log = LogUtils::s_commonLog;

AsyncEmailWriter::AsyncEmailWriter(Email& email)
: EmailWriter(email),
  doneSignal(this),
  m_paused(false),
  m_ioFactory(new GIOChannelWrapperFactory()),
  m_bodyPartWrittenSlot(this, &AsyncEmailWriter::WritePartDone)
{
}

AsyncEmailWriter::~AsyncEmailWriter()
{
}

void AsyncEmailWriter::WriteEmail(EmailWrittenSignal::SlotRef doneSlot)
{
	doneSignal.connect(doneSlot);
	
	MojLogDebug(s_log, "started writing email");
	
	try {
		StartEmail();
	} CATCH_AS_EMAIL_FAILED
}

void AsyncEmailWriter::SetAsyncIOChannelFactory(const boost::shared_ptr<AsyncIOChannelFactory> &factory)
{
	m_ioFactory = factory;
}

void AsyncEmailWriter::SetPartList(const EmailPartList &parts)
{
	m_partList = parts; // shallow copy
}

void AsyncEmailWriter::StartEmail()
{
	m_altParts.clear();
	m_mixedParts.clear();

	// Sort parts by type
	BOOST_FOREACH(const EmailPartPtr& part, m_partList) {
		if(part->IsBodyPart()) {
			m_altParts.push_back(part);
		} else {
			if(part->IsInline() && part->GetMimeType() == "text/calendar") {
				// Special case for calendar ics files -- treat as a body part
				m_altParts.push_back(part);
			} else {
				m_mixedParts.push_back(part);
			}
		}
	}

	m_currentBoundary = m_boundary;

	if(m_mixedParts.size() > 0) {
		// Got some attachments
		WriteEmailHeader("multipart/mixed", m_boundary);

		if(m_altParts.size() > 1) {
			WriteAlternativeHeader(m_boundary, m_altBoundary);
			m_currentBoundary = m_altBoundary;
		} else {
			// Don't bother with an alternatives section if there's only one body
			m_mixedParts.insert(m_mixedParts.begin(), m_altParts.begin(), m_altParts.end());
			m_altParts.clear();
		}
	} else {
		// Just body parts
		WriteEmailHeader("multipart/alternative", m_altBoundary);
		m_currentBoundary = m_altBoundary;
	}

	WriteParts();
}

void AsyncEmailWriter::WriteParts()
{
	if(!m_altParts.empty()) {
		// Need to write alternative body parts
		m_partsToWrite = m_altParts;
		m_altParts.clear();

		WriteQueuedParts();
	} else if(!m_mixedParts.empty()) {
		// Done writing body parts
		if(m_currentBoundary == m_altBoundary) {
			WriteBoundary(m_currentBoundary, true);

			m_currentBoundary = m_boundary;
		}

		m_partsToWrite = m_mixedParts;
		m_mixedParts.clear();

		WriteQueuedParts();
	} else {
		// Done
		WriteBoundary(m_currentBoundary, true);

		EndEmail();
	}
}

MojRefCountedPtr<PartWriter> AsyncEmailWriter::GetPartWriter(const string& filePath)
{
	MojRefCountedPtr<FilePartWriter> partWriter(new FilePartWriter());
	partWriter->SetChannelFactory(m_ioFactory);
	partWriter->OpenFile(filePath);

	return partWriter;
}

void AsyncEmailWriter::WriteQueuedParts()
{
	while(!m_partsToWrite.empty()) {
		// Pop a part from the list
		EmailPartPtr part = m_partsToWrite.front();
		m_partsToWrite.erase(m_partsToWrite.begin());

		const std::string filePath = part->GetLocalFilePath();

		bool canSkip = !part->IsBodyPart();

		try {
			if(filePath.empty()) {
				throw MailException("part path is empty", __FILE__, __LINE__);
			}

			m_partWriter = GetPartWriter(filePath);

			OutputStreamPtr os = GetPartOutputStream(*part);

			string emailIdString = AsJsonString(m_email.GetId());
			string partIdString = AsJsonString(part->GetId());

			MojLogDebug(s_log, "started writing part %s of email %s", partIdString.c_str(), emailIdString.c_str());

			WritePartHeader(*part, m_currentBoundary);

			m_partWriter->WriteToStream(os, m_bodyPartWrittenSlot);

			// Exit function
			return;
		} catch(const std::exception& e) {
			if(canSkip) {
				continue; // get next part (if any)
			} else {
				throw e;
			}
		}
	}

	// Go back to WriteParts
	WriteParts();
}

MojErr AsyncEmailWriter::WritePartDone(const std::exception* e)
{
	try {
		WritePartFooter();

		m_oldPartWriters.push_back(m_partWriter);
		m_partWriter.reset();

		WriteQueuedParts();
	} CATCH_AS_EMAIL_FAILED

	return MojErrNone;
}

void AsyncEmailWriter::EndEmail()
{
	MojLogDebug(s_log, "done writing email");
	
	doneSignal.fire(NULL);
	// FIXME handle error
}

void AsyncEmailWriter::PauseWriting()
{
	m_paused = true;

	if(m_partWriter.get()) {
		m_partWriter->PauseWriting();
	}
}

void AsyncEmailWriter::ResumeWriting()
{
	m_paused = false;

	if(m_partWriter.get()) {
		m_partWriter->ResumeWriting();
	}
}

void AsyncEmailWriter::AbortWriting()
{
	if(m_partWriter.get()) {
		m_partWriter->AbortWriting();
	}
}

void AsyncEmailWriter::WriteEmailFailed(const std::exception &e)
{
	MojLogError(s_log, "error writing email: %s", e.what());

	doneSignal.fire(&e);
}

FilePartWriter::FilePartWriter()
: m_partChannelReadableSlot(this, &FilePartWriter::HandlePartDataAvailable)
{
}

FilePartWriter::~FilePartWriter()
{
}

void FilePartWriter::SetChannelFactory(const boost::shared_ptr<AsyncIOChannelFactory>& factory)
{
	m_ioFactory = factory;
}

void FilePartWriter::OpenFile(const std::string& filename)
{
	std::string filePath = filename;

	StringUtils::SanitizeFilePath(filePath);

	bool isPathAllowed = SBIsPathAllowed(filePath.c_str(), "" /* publicly-accessible files only */, SB_READ);

	// FIXME: exempt /tmp until people stop using it
	if(!isPathAllowed) {
		// NOTE: this allocates memory which must be freed.
		char* tempPath = SBCanonicalizePath(filePath.c_str());

		std::string canonicalPath;

		if(tempPath) {
			if(tempPath)
				canonicalPath.assign(tempPath);

			free(tempPath);
		}

		// NOTE: this doesn't check for malicious symlinks
		if(boost::starts_with(canonicalPath, "/tmp/")) {
			isPathAllowed = true;
		}
	}

	// Check if the path is valid
	if(!isPathAllowed) {
		MojLogWarning(s_log, "attempted to access part path outside of sandbox");
		throw MailException("invalid part file path: permission denied", __FILE__, __LINE__);
	}

	// Create channel
	try {
		if(!m_ioFactory.get()) {
			m_ioFactory.reset(new GIOChannelWrapperFactory());
		}

		m_partChannel = m_ioFactory->OpenFile(filePath.c_str(), "r");
	} catch(const std::exception& e) {
		MojLogError(s_log, "error opening file %s: %s", filePath.c_str(), e.what());

		throw e;
	}
}

void FilePartWriter::WriteToStream(const OutputStreamPtr& outputStream, PartWrittenSignal::SlotRef doneSlot)
{
	m_outputStream = outputStream;

	m_doneSignal.connect(doneSlot);

	try {
		m_partChannel->WatchReadable(m_partChannelReadableSlot);
	} CATCH_AS_PART_FAILED
}

void FilePartWriter::PauseWriting()
{
	try {
		if(!m_paused && m_partChannel.get()) {
			m_paused = true;
			m_partChannel->UnwatchReadable(m_partChannelReadableSlot);
		}
	} CATCH_AS_PART_FAILED
}

void FilePartWriter::ResumeWriting()
{
	try {
		if(m_paused && m_partChannel.get()) {
			// Try reading some data on the spot
			m_paused = false;

			if(ReadPartData()) {
				// If we still need more data, start watching the channel
				m_partChannel->WatchReadable(m_partChannelReadableSlot);
			}
		}
	} CATCH_AS_PART_FAILED
}

void FilePartWriter::AbortWriting()
{
	m_partChannelReadableSlot.cancel();

	if(m_partChannel.get()) {
		m_partChannel->Shutdown();
	}

	MojLogDebug(s_log, "aborted writing part");
}

MojErr FilePartWriter::HandlePartDataAvailable()
{
	// Note: exceptions will be handled inside ReadPartdat()
	ReadPartData();

	return MojErrNone;
}

bool FilePartWriter::ReadPartData()
{
	assert(m_outputStream.get());

	const size_t BUF_SIZE = 8196;
	char buf[BUF_SIZE]; // FIXME: Should this be allocated on the heap instead?
	
	bool eof = false;

	try {
		while(true) {
			size_t bytesRead = m_partChannel->Read(buf, BUF_SIZE, eof);
			MojLogDebug(s_log, "part read %d bytes from file", bytesRead);

			if(bytesRead > 0) {
				m_outputStream->Write(buf, bytesRead);
			} else if(eof) {
				// End of file
				MojLogDebug(s_log, "part ended");
				m_outputStream->Flush();

				PartFinished();
				return false; // doesn't need more data
			} else {
				// No more data available right now
				return true; // needs more data
			}
			
			if(m_paused) {
				MojLogDebug(s_log, "part paused reading data");
				return false; // doesn't need more data
			}
		}
	} CATCH_AS_PART_FAILED
	
	return true;
}
void FilePartWriter::PartFinished()
{
	try {
		// Cleanup channel
		if(!m_paused)
			m_partChannel->UnwatchReadable(m_partChannelReadableSlot);
		m_partChannel->Shutdown();

		MojLogDebug(s_log, "done writing part");

		m_doneSignal.fire(NULL);
	} CATCH_AS_PART_FAILED
}

void FilePartWriter::WritePartFailed(const std::exception& e)
{
	m_partChannelReadableSlot.cancel();

	if(m_partChannel.get()) {
		m_partChannel->Shutdown();
	}
}

InputStreamPartWriter::InputStreamPartWriter(const InputStreamPtr& inputStream)
: InputStreamSink(inputStream)
{
}

InputStreamPartWriter::~InputStreamPartWriter()
{
}

void InputStreamPartWriter::WriteToStream(const OutputStreamPtr& outputStream, PartWrittenSignal::SlotRef doneSlot)
{
	m_outputStream = outputStream;

	m_doneSignal.connect(doneSlot);

	ResumeWriting();
}

void InputStreamPartWriter::PauseWriting()
{
	m_paused = true;

	if(m_source.get()) {
		m_source->DoneReading();
	}
}

void InputStreamPartWriter::ResumeWriting()
{
	m_paused = false;

	if(m_source.get()) {
		m_source->StartReading();
	}
}

size_t InputStreamPartWriter::HandleData(const char* data, size_t length, bool eof)
{
	m_outputStream->Write(data, length);

	if(eof) {
		m_doneSignal.fire(NULL);
	}

	return length;
}
