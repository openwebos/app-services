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

#include "mimeparser/MimeEmailParser.h"
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/foreach.hpp>
#include <iostream>
#include "mimeparser/Rfc822Tokenizer.h"
#include "mimeparser/EmailHeaderFieldParser.h"

using namespace std;

MimeEmailParser::MimeEmailParser(ParseEventHandler& handler)
: m_handler(handler), m_state(State_Initial), m_isMultipart(false), m_paused(false)
{
}

MimeEmailParser::~MimeEmailParser()
{
}

const char* MimeEmailParser::FindEndOfLine(const char* start, const char* end, bool isEOF, const char*& newlinePos)
{
	for(const char* pos = start; pos < end; ++pos) {
		if(pos[0] == '\r') {
			if(end - pos > 0 && pos[1] == '\n') {
				newlinePos = pos;
				return pos + 2; // \r\n
			} else if (isEOF) {
				newlinePos = pos;
				return pos + 1; // \r
			}
		} else if(pos[0] == '\n') {
			newlinePos = pos;
			return pos + 1;
		}
	}

	newlinePos = NULL;
	return NULL;
}

size_t MimeEmailParser::ParseData(const char* data, size_t length, bool isEOF)
{
	//cerr << "{ParseData: " << boost::replace_all_copy(std::string(data, length), "\r", "") << "}" << endl;

	const char* pos = data;
	const char* end = data + length;

	// pair of offset, length
	const char* bodyStart = NULL;
	const char* bodyEnd = NULL;

	size_t totalBytesProcessed = 0;

	m_paused = false;

	// Find line endings
	while (!m_paused) {
		const char* newlinePos = NULL;
		const char* endOfLine = FindEndOfLine(pos, end, isEOF, newlinePos);

		if (endOfLine) {
			//cerr << "line length = " << size_t(endOfLine - pos) << endl;
			totalBytesProcessed = endOfLine - data;

			if (m_state == State_Body) {
				// Add lines to buffer until we hit boundary or end of current input
				BoundaryMatchResult result;
				if (MatchBoundary(m_boundaryStack, pos, endOfLine - pos, result)) {
					// Process buffer
					if (bodyStart && bodyEnd) {
						m_handler.HandleBodyData(bodyStart, bodyEnd - bodyStart);
					}

					// Clear body buffer
					bodyStart = bodyEnd = NULL;

					// Process the boundary
					ProcessBoundaryMatch(result);
				} else {
					if (bodyStart == NULL) {
						// Set start of body here
						bodyStart = pos;
					}
					// Set end of body
					bodyEnd = endOfLine;
				}
			} else {
				// TODO assert there's no buffered body

				// Parse one line at a time
				ParseLine( pos, (size_t) (endOfLine - pos) );
			}

			pos = endOfLine;
		} else {
			break;
		}
	}

	// Handle any buffered body data
	if (bodyStart && bodyEnd) {
		m_handler.HandleBodyData(bodyStart, bodyEnd - bodyStart);
	}

	return totalBytesProcessed;
}

void MimeEmailParser::ParseLine(const string& line)
{
	ParseLine(line.data(), line.length());
}

void MimeEmailParser::ParseLine(const char* lineData, size_t lineLength)
{
	if (CheckBoundaries(lineData, lineLength)) {
		return;
	}

	if (m_state == State_Body) {
		//cout << "BODY LINE: " << line;
		m_handler.HandleBodyData(lineData, lineLength);
	} else if (m_state == State_WaitingForHeader || m_state == State_Header) {
		size_t headerLineLength = lineLength;

		// Strip newline chars from end
		for (; headerLineLength > 0; --headerLineLength) {
			char lastChar = lineData[headerLineLength - 1];
			if (lastChar != '\r' && lastChar != '\n') {
				break;
			}
		}

		if (m_state == State_WaitingForHeader) {
			if (headerLineLength == 0) {
				// no header yet; ignore this line
				return;
			} else {
				BeginHeaders();
				// and then handle the header line as normal
			}
		}

		if (headerLineLength == 0) {
			// Blank line means end of headers

			// Parse last header line
			if (!m_lastLineBuffer.empty()) {
				ParseHeaderLine(m_lastLineBuffer);
				m_lastLineBuffer.clear();
			}

			EndHeaders();
		} else if (lineData[0] == ' ' || lineData[0] == '\t') {
			// Continuation
			m_state = State_Header;
			m_lastLineBuffer.append(lineData, headerLineLength);
		} else {
			// New header line; parse last header line
			if (!m_lastLineBuffer.empty()) {
				ParseHeaderLine(m_lastLineBuffer);
			}

			// replace line buffer with new line
			m_lastLineBuffer.assign(lineData, headerLineLength);
		}

	} else if (m_state == State_WaitingForBoundary) {
		// Waiting for the start of a part
	} else if (m_state == State_Initial) {
		// Waiting for start of email
	}
}

void MimeEmailParser::SetState(ParserState newState)
{
	//cerr << "changing state to " << newState << endl;
	m_state = newState;
}

void MimeEmailParser::BeginEmail()
{
	m_handler.HandleBeginEmail();
	BeginPart();
}

void MimeEmailParser::EndEmail(bool incomplete)
{
	// Pop all parts except the outermost one (which has no boundary terminator)
	PopBoundaries(0);

	// Pop outermost part
	EndPart(incomplete);

	m_handler.HandleEndEmail();
}

void MimeEmailParser::BeginHeaders()
{
	SetState(State_Header);
	m_isMultipart = false;

	m_handler.HandleBeginHeaders();
}

/* Called when finished parsing headers for the email or a part */
void MimeEmailParser::EndHeaders()
{
	m_handler.HandleEndHeaders(false /* headers parsed successfully */);

	if (m_isMultipart) {
		SetState(State_WaitingForBoundary);
	} else {
		SetState(State_Body);
		m_handler.HandleBeginBody();
	}
}

void MimeEmailParser::ParseHeaderLine(const string& line)
{
	string fieldName;

	size_t colonIndex = line.find(':');
	if (!colonIndex) {
		// FIXME throw exception
		return;
	}

	fieldName = line.substr(0, colonIndex);
	boost::trim(fieldName);

	string fieldValue = line.substr(colonIndex + 1);
	boost::trim(fieldValue);

	m_handler.HandleHeader(fieldName, fieldValue);

	// Parse content-type header so we can find the boundaries
	if (boost::iequals(fieldName, "content-type")) {
		EmailHeaderFieldParser parser;

		string type, subtype;
		EmailHeaderFieldParser::ParameterMap params;
		parser.ParseContentTypeField(fieldValue, type, subtype, params);

		m_handler.HandleContentType(type, subtype, params);

		if (boost::iequals(type, "multipart")) {
			string boundary = params["boundary"];

			if (!boundary.empty()) {
				m_isMultipart = true;
				m_boundaryStack.push_back( BoundaryState(boundary) );
			}
		}
	}
}

void MimeEmailParser::BeginPart()
{
	m_handler.HandleBeginPart();
	SetState(State_WaitingForHeader);
}

void MimeEmailParser::EndPart(bool incomplete)
{
	switch (m_state) {
	case State_Header: m_handler.HandleEndHeaders(true /* incomplete headers */); break;
	case State_Body: m_handler.HandleEndBody(incomplete); break;
	default: break;
	}

	m_handler.HandleEndPart();
	SetState(State_WaitingForBoundary);
}

// Find a boundary from the given list of boundaries
bool MimeEmailParser::MatchBoundary(const std::vector<BoundaryState>& boundaries, const char* lineData, size_t lineLength, BoundaryMatchResult& result)
{
	result.matchIndex = -1;
	result.isEnd = false;

	if (lineLength < 3 || lineData[0] != '-' || lineData[1] != '-') {
		return false;
	} else {
		for (int i = boundaries.size() - 1; i >= 0; --i) {
			const string& boundary = boundaries[i].boundary;

			if (boundary.compare(0, boundary.length(), lineData + 2, std::min(boundary.length(), lineLength - 2)) == 0) {
				result.matchIndex = i;

				result.isEnd = (lineLength >= boundary.length() + 4
						&& lineData[boundary.length() + 2] == '-'
						&& lineData[boundary.length() + 3] == '-');

				return true;
			}
		}

		return false;
	}
}

bool MimeEmailParser::CheckBoundaries(const char* lineData, size_t lineLength)
{
	if (lineLength > 2 && lineData[0] == '-' && lineData[1] == '-') {
		BoundaryMatchResult result;
		if (MatchBoundary(m_boundaryStack, lineData, lineLength, result)) {
			ProcessBoundaryMatch(result);
			return true;
		}
	}
	return false;
}

void MimeEmailParser::ProcessBoundaryMatch(BoundaryMatchResult& result)
{
	if (result.matchIndex >= 0) {
		// Found a boundary
		// If this isn't the topmost boundary, end any subparts current being parsed
		PopBoundaries(result.matchIndex + 1);

		BoundaryState& currentBoundaryState = m_boundaryStack[result.matchIndex];

		// End the current part (if it's been started)
		if (currentBoundaryState.partStarted) {
			EndPart(false);
		}

		if (!result.isEnd) {
			// Start new subpart
			currentBoundaryState.partStarted = true; // started at least one part
			BeginPart();
		} else {
			currentBoundaryState.partStarted = false;
		}
	}
}

// End parts corresponding to boundaries starting from (and including) startIndex
void MimeEmailParser::PopBoundaries(int startIndex)
{
	for (int i = m_boundaryStack.size() - 1; i >= startIndex; --i) {
		bool partStarted = m_boundaryStack.at(i).partStarted;
		m_boundaryStack.erase(m_boundaryStack.begin() + i);

		if (partStarted) {
			bool incomplete = (i != startIndex);
			EndPart(incomplete);
		}
	}
}
