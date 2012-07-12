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

#include "async/FileCacheResizerOutputStream.h"
#include "CommonPrivate.h"

FileCacheResizerOutputStream::FileCacheResizerOutputStream(const OutputStreamPtr& sink, FileCacheClient& fileCacheClient, const std::string& path, MojInt64 initialSize)
: ChainedOutputStream(sink),
  m_fileCacheClient(fileCacheClient),
  m_path(path),
  m_fileCacheBytesAllocated(initialSize),
  m_bytesWritten(0),
  m_resizeRequestPending(false),
  m_closePending(false),
  m_requestedBytes(0),
  m_fullSignal(this),
  m_writeableSignal(this),
  m_resizeSlot(this, &FileCacheResizerOutputStream::ResizeResponse)
{
}

FileCacheResizerOutputStream::~FileCacheResizerOutputStream()
{
}

MojInt64 FileCacheResizerOutputStream::SpaceLeft() const
{
	return std::max(m_fileCacheBytesAllocated - m_bytesWritten, 0LL);
}

// This tries to write out as much as possible to the sink output stream
void FileCacheResizerOutputStream::FlushBuffer()
{
	size_t bytesToWrite = std::min(SpaceLeft(), (MojInt64) m_buffer.size());
	ChainedOutputStream::Write(m_buffer.data(), bytesToWrite);
	m_buffer.erase(0, bytesToWrite);

	m_bytesWritten += bytesToWrite;

	if (!m_closePending && SpaceLeft() > 0) {
		m_writeableSignal.call();
	}
}

void FileCacheResizerOutputStream::Write(const char* src, size_t length)
{
	if (m_error.errorCode != MailError::NONE) {
		// not good
		return;
	}

	//fprintf(stderr, "writing %d bytes; file at %lld/%lld\n", length, m_bytesWritten, m_fileCacheBytesAllocated);

	if (SpaceLeft() > 0) {
		// Write existing buffer
		FlushBuffer();

		// Write new stuff
		if (SpaceLeft() > 0) {
			size_t bytesToWrite = std::min((MojInt64) length, SpaceLeft());
			ChainedOutputStream::Write(src, bytesToWrite);
			src += bytesToWrite;
			length -= bytesToWrite;
			m_bytesWritten += bytesToWrite;
		}
	}

	// Put anything left over in the buffer and try to expand the filecache
	if (length > 0) {
		m_buffer.append(src, length);

		RequestResize();

		m_fullSignal.call();
	}
}

void FileCacheResizerOutputStream::RequestResize()
{
	if (!m_resizeRequestPending) {
		m_resizeSlot.cancel();
		m_resizeRequestPending = true;

		MojInt64 newSize = m_fileCacheBytesAllocated + m_buffer.size();

		MojInt64 extraSpace = newSize * 0.75; // request an extra 75%
		extraSpace = std::max(extraSpace, 16 * 1024LL); // grow by at least 16K at a time
		extraSpace = std::min(extraSpace, 1024 * 1024LL); // but no more than 1MB at a time

		newSize += extraSpace;

		m_requestedBytes = newSize;

		MojLogInfo(LogUtils::s_commonLog, "resizing file cache entry '%s' to %lld bytes", m_path.c_str(), newSize);

		m_fileCacheClient.ResizeCacheObject(m_resizeSlot, m_path.c_str(), newSize);
	}
}

MojErr FileCacheResizerOutputStream::ResizeResponse(MojObject& response, MojErr err)
{
	// cancel slot just in case
	m_resizeSlot.cancel();
	m_resizeRequestPending = false;

	try {
		ErrorToException(err);

		err = response.getRequired("newSize", m_fileCacheBytesAllocated);
		ErrorToException(err);

		MojLogDebug(LogUtils::s_commonLog, "successfully resized file cache entry '%s'", m_path.c_str());

		// Write buffer
		FlushBuffer();

		// If the buffer isn't empty, need to resize again
		if (!m_buffer.empty()) {
			RequestResize();
		} else {
			if (m_closePending) {
				// Okay, we can close now
				m_closePending = false;
				ChainedOutputStream::Close();
			}
		}
	} catch (const std::exception& e) {
		MojLogError(LogUtils::s_commonLog, "error resizing file cache entry to %lld bytes: %s", m_requestedBytes, e.what());

		m_error = MailError::ErrorInfo(MailError::INTERNAL_ERROR, e.what());

		if (m_closePending) {
			// Allow the stream to close in case anyone's waiting on us
			m_closePending = false;
			ChainedOutputStream::Close();
		} else {
			m_writeableSignal.call();
		}
	}
	return MojErrNone;
}

void FileCacheResizerOutputStream::Flush(FlushType flushType)
{
	// ignore flush requests
}

void FileCacheResizerOutputStream::Close()
{
	if (!m_buffer.empty() && m_error.errorCode == MailError::NONE) {
		// Need to write out buffer before closing
		m_closePending = true;
		RequestResize();
	} else {
		ChainedOutputStream::Close();
	}
}
