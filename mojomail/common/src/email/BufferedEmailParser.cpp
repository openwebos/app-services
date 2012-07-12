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

#include "email/BufferedEmailParser.h"

BufferedEmailParser::BufferedEmailParser()
: AsyncEmailParser(),
  m_endOfStream(false),
  m_seenCR(false)
{
}

BufferedEmailParser::~BufferedEmailParser()
{
}

void BufferedEmailParser::End()
{
	m_endOfStream = true;

	if (!m_paused) {
		FlushBuffer();
	}
}

bool BufferedEmailParser::FeedChar(char c)
{
	m_buffer.push_back(c);

	if (!m_isMimeStream) {
		if (!m_paused && m_buffer.size() >= 4096) {
			FlushBuffer();
		}
	} else {
		if (!m_paused) {
			if (c == '\n' || m_seenCR) {
				// complete line available
				FlushBuffer();
			} else if (c == '\r') {
				m_seenCR = true; // might be \r or \r\n
			}

			m_seenCR = false;
		}
	}

	return !m_paused;
}

void BufferedEmailParser::Unpause()
{
	if (m_paused) {
		m_paused = false;

		FlushBuffer();

		if (!m_paused) {
			m_readySignal.call();
		}
	}
}

void BufferedEmailParser::FlushBuffer()
{
	if (m_isMimeStream) {
		// parse full lines out of buffer
		size_t bytesHandled = ParseData(m_buffer.data(), m_buffer.size(), m_endOfStream);
		m_buffer.erase(0, bytesHandled);

		// update seenCR
		m_seenCR = m_buffer.empty() ? false : (*m_buffer.end() == '\r');
	} else {
		ParseData(m_buffer.data(), m_buffer.size(), m_endOfStream);
		m_buffer.clear();
	}

	if (m_endOfStream && !m_paused) {
		AsyncEmailParser::End();
	}
}
