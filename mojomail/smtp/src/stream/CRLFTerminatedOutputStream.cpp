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

#include "stream/CRLFTerminatedOutputStream.h"
#include "exceptions/MailException.h"

CRLFTerminatedOutputStream::CRLFTerminatedOutputStream(const OutputStreamPtr& sink)
: ChainedOutputStream(sink),
	m_penultimateChar('\0'),
	m_ultimateChar('\0')
{
}

CRLFTerminatedOutputStream::~CRLFTerminatedOutputStream()
{
}

void CRLFTerminatedOutputStream::Write(const char* src, size_t length)
{
	m_sink->Write(src, length);
	
	// Update memory of last two bytes seen.
	
	if (length >= 2) {
		m_penultimateChar = src[length-2];
		m_ultimateChar = src[length-1];
	} else if (length == 1) {
		m_penultimateChar = m_ultimateChar;
		m_ultimateChar = src[length-1];
	}
}

void CRLFTerminatedOutputStream::Flush()
{
	int skip;

	if (m_penultimateChar == '\r' && m_ultimateChar == '\n') {
		skip = 2;
	} else if (m_ultimateChar == '\r') {
		skip = 1;
	} else {
		skip = 0;
	}
	
	m_sink->Write("\r\n" + skip, 2 - skip);
	m_sink->Flush();
}
