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


#ifndef PARSER3_H_
#define PARSER3_H_

#include <string>
#include <vector>
#include <map>
#include <boost/shared_ptr.hpp>

class ParseEventHandler
{
public:
	virtual void HandleBeginEmail() = 0;
	virtual void HandleEndEmail() = 0;

	virtual void HandleBeginPart() = 0;
	virtual void HandleEndPart() = 0;

	virtual void HandleBeginHeaders() = 0;
	virtual void HandleEndHeaders(bool incomplete) = 0;

	// Begin/end body (excluding multi-part bodies)
	virtual void HandleBeginBody() = 0;
	virtual void HandleBodyData(const char* data, size_t length) = 0;
	virtual void HandleEndBody(bool incomplete) = 0;

	virtual void HandleContentType(const std::string& type, const std::string& subtype, const std::map<std::string, std::string>& headers) = 0;
	virtual void HandleHeader(const std::string& fieldName, const std::string& line) = 0;

protected:
	ParseEventHandler() {}
	virtual ~ParseEventHandler() {}
};

class MimeEmailParser
{
public:
	MimeEmailParser(ParseEventHandler& handler);
	virtual ~MimeEmailParser();

	// Set the list of fields that should be handled by the parser.
	// Any other field will be ignored to improve performance.
	// The content-type field will always be parsed.
	void SetRelevantFields(const std::vector<std::string>& fieldNames);

	// Begin parsing email
	void BeginEmail();

	// Parse a block of data
	// Returns the number of bytes read
	size_t ParseData(const char* data, size_t length, bool isEOF);

	// Force ParseData to stop parsing the buffer
	void PauseParseData() { m_paused = true; }

	// Feed a line of data to the parser
	void ParseLine(const char* data, size_t length);
	void ParseLine(const std::string& line);

	// End parsing email (end of input)
	void EndEmail(bool incomplete = false);

protected:
	enum ParserState {
		State_Initial,
		State_WaitingForHeader,
		State_Header,
		State_WaitingForBoundary,
		State_Body
	};

	enum BoundaryType {
		Boundary_Other,
		Boundary_BeginPart,
		Boundary_End
	};

	struct BoundaryState {
		BoundaryState(const std::string& boundary) : boundary(boundary), partStarted(false) {}

		std::string		boundary;
		bool			partStarted;
	};

	void ParseHeaderLine(const std::string& line);
	void ParseBodyLine(const std::string& line);

	void BeginPart();
	void EndPart(bool complete);

	void BeginHeaders();
	void EndHeaders();

	void SetState(ParserState newState);

	void CheckPartBeginEnd(const std::string& line);

	const char* FindEndOfLine(const char* start, const char* end, bool isEOF, const char*& newlinePos);

	struct BoundaryMatchResult
	{
		int matchIndex; // -1 for no match
		bool isEnd;
	};

	static bool MatchBoundary(const std::vector<BoundaryState>& boundaries, const char* lineData, size_t lineLength, BoundaryMatchResult& result);

	bool CheckBoundaries(const char* lineData, size_t lineLength);
	void ProcessBoundaryMatch(BoundaryMatchResult& result);
	void PopBoundaries(int startIndex);

	ParseEventHandler&	m_handler;
public:
	ParserState			m_state;

	bool				m_isMultipart;
	std::vector<BoundaryState>	m_boundaryStack;

	std::string			m_lastLineBuffer;
	bool				m_paused;
};

#endif /* PARSER3_H_ */
