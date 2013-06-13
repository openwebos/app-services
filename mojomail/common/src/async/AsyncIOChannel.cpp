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

#include "async/AsyncIOChannel.h"
#include <boost/shared_ptr.hpp>
#include <glib.h>
#include "exceptions/MailException.h"
#include "exceptions/ExceptionUtils.h"
#include "CommonPrivate.h"

MojLogger& AsyncIOChannel::s_log = LogUtils::s_commonLog;

AsyncIOChannel::AsyncIOChannel()
: m_readers(0),
  m_writers(0),
  m_readableSignal(this),
  m_writeableSignal(this),
  m_closedSignal(this),
  m_closed(false)
{
}

AsyncIOChannel::~AsyncIOChannel()
{
}

void AsyncIOChannel::SetException(const std::exception& e)
{
	m_exception = ExceptionUtils::CopyException(e);
	m_errorInfo = ExceptionUtils::GetErrorInfo(e);
}

const boost::exception_ptr& AsyncIOChannel::GetException() const
{
	return m_exception;
}

MailError::ErrorInfo AsyncIOChannel::GetErrorInfo() const
{
	return m_errorInfo;
}

void AsyncIOChannel::WatchReadable(ReadableSignal::SlotRef slot)
{
	if(m_readers < 1) {
		// Register a callback when data is available for reading without blocking
		SetWatchReadable(true);
	}

	++m_readers;
	m_readableSignal.connect(slot);
}

void AsyncIOChannel::UnwatchReadable(ReadableSignal::SlotRef slot)
{
	--m_readers;
	// Cancel callback
	if(m_readers <= 0) {
		SetWatchReadable(false);
	}

	slot.cancel();
}

void AsyncIOChannel::WatchWriteable(WriteableSignal::SlotRef slot)
{
	if(m_writers < 1) {
		SetWatchWriteable(true);
	}

	++m_writers;
	m_writeableSignal.connect(slot);
}

void AsyncIOChannel::UnwatchWriteable(WriteableSignal::SlotRef slot)
{
	--m_writers;

	// Cancel callback
	if(m_writers <= 0) {
		SetWatchWriteable(false);
	}

	slot.cancel();
}

void AsyncIOChannel::WatchClosed(ClosedSignal::SlotRef slot)
{
	m_closedSignal.connect(slot);
}

void AsyncIOChannel::UnwatchClosed(ClosedSignal::SlotRef slot)
{
	slot.cancel();
}

// Should be overridden
void AsyncIOChannel::SetWatchReadable(bool watch)
{
}

// Should be overridden
void AsyncIOChannel::SetWatchWriteable(bool watch)
{
}

AsyncIOChannelFactory::AsyncIOChannelFactory()
{
}

AsyncIOChannelFactory::~AsyncIOChannelFactory()
{
}
