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

#ifndef ASYNCEMAILWRITER_H_
#define ASYNCEMAILWRITER_H_

#include <glib.h>
#include <queue>
#include "data/CommonData.h"
#include "email/EmailWriter.h"
#include "async/AsyncIOChannel.h"
#include "async/AsyncWriter.h"

/* PartWriter interface */
class PartWriter : public AsyncWriter, public virtual MojSignalHandler
{
public:
	typedef MojSignal<const std::exception*> PartWrittenSignal;

protected:
	PartWriter() : m_paused(false), m_doneSignal(this) {}
	virtual ~PartWriter() {}

public:
	virtual void WriteToStream(const OutputStreamPtr& outputStream, PartWrittenSignal::SlotRef doneSlot) = 0;

protected:
	OutputStreamPtr		m_outputStream;

	bool				m_paused;
	PartWrittenSignal	m_doneSignal;
};

/* Copies a file to an output stream.
 * FIXME: Should eventually be replaced by InputStreamPartWriter.
 */
class FilePartWriter : public PartWriter
{
public:
	typedef MojSignal<const std::exception*> PartWrittenSignal;

	FilePartWriter();
	virtual ~FilePartWriter();

	void SetChannelFactory(const boost::shared_ptr<AsyncIOChannelFactory>& factory);

	void OpenFile(const std::string& fileName);
	void WriteToStream(const OutputStreamPtr& outputStream, PartWrittenSignal::SlotRef doneSlot);

	// AsyncWriter methods
	void PauseWriting();
	void ResumeWriting();
	void AbortWriting();

protected:
	static MojLogger& s_log;

	// Callback when part data is available
	MojErr HandlePartDataAvailable();

	// Read data from channel. Returns true if we want more data (continue watching)
	bool ReadPartData();

	// Called when done
	void PartFinished();

	// Error writing part
	void WritePartFailed(const std::exception& e);

	boost::shared_ptr<AsyncIOChannelFactory>		m_ioFactory;

	MojRefCountedPtr<AsyncIOChannel>	m_partChannel;			// must use MojRefCountedPtr

	AsyncIOChannel::ReadableSignal::Slot<FilePartWriter>		m_partChannelReadableSlot;
};

/* Copies an input stream to the output stream */
class InputStreamPartWriter : public PartWriter, public InputStreamSink
{
public:
	InputStreamPartWriter(const InputStreamPtr& inputStream);
	virtual ~InputStreamPartWriter();

	// PartWriter methods
	virtual void WriteToStream(const OutputStreamPtr& outputStream, PartWrittenSignal::SlotRef doneSlot);

	// AsyncWriter methods
	void PauseWriting();
	void ResumeWriting();

protected:
	size_t HandleData(const char* data, size_t length, bool eof);
};

class AsyncEmailWriter : public EmailWriter, public AsyncWriter, public MojSignalHandler
{
public:
	typedef MojSignal<const std::exception*> EmailWrittenSignal;
	
	AsyncEmailWriter(Email& email);
	virtual ~AsyncEmailWriter();
	
	/**
	 * Used for testing.
	 */
	void SetAsyncIOChannelFactory(const boost::shared_ptr<AsyncIOChannelFactory> &factory);
	
	/**
	 * Sets the part list.
	 * 
	 * The part list should already have been filtered for smart forward, etc.
	 */
	void SetPartList(const EmailPartList& parts);
	
	/**
	 * Write an email to the current output stream (set with setOutputStream)
	 * The done signal will be called after it finishes writing the email.
	 * 
	 * Must call SetPartList before writing the email.
	 * After calling WriteEmail, do not call it again until after the done signal is called.
	 */
	void WriteEmail(EmailWrittenSignal::SlotRef doneSlot);
	
	// Pause writing parts
	void PauseWriting();
	void ResumeWriting();
	void AbortWriting();
	
protected:
	static MojLogger& s_log;
	
	void StartEmail();

	virtual MojRefCountedPtr<PartWriter> GetPartWriter(const std::string& filePath);

	void WriteParts();
	void WriteQueuedParts();
	MojErr WritePartDone(const std::exception* e);

	void WriteAttachments();
	MojErr WriteAttachmentDone(const std::exception* e);

	void PartFinished();
	void EndEmail();
	
	void WriteEmailFailed(const std::exception &exc);
	
	EmailWrittenSignal doneSignal;
	
	EmailPartList	m_partList;

	// Parts queued to be written
	EmailPartList	m_altParts;
	EmailPartList	m_mixedParts;
	EmailPartList	m_partsToWrite;

	bool		m_paused;

	std::string		m_currentBoundary;

	// Used to construct AsyncIOChannel objects for files
	boost::shared_ptr<AsyncIOChannelFactory>		m_ioFactory;

	MojRefCountedPtr<PartWriter>	m_partWriter;

	// Workaround to keep the channel in memory
	std::vector< MojRefCountedPtr<PartWriter> >		m_oldPartWriters;

	FilePartWriter::PartWrittenSignal::Slot<AsyncEmailWriter>	m_bodyPartWrittenSlot;
};

#endif /*ASYNCEMAILWRITER_H_*/
