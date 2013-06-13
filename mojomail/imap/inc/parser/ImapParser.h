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

#ifndef IMAPPARSER_H_
#define IMAPPARSER_H_

#include "parser/Parser.h"
#include <cassert>
#include <map>
#include <string>
#include <stack>
#include <vector>
#include "core/MojRefCount.h"
#include "parser/ParserTypes.h"
#include <locale>

class CallStackEntry;
class Rfc3501Tokenizer;

class ImapParser
{
public:
	ImapParser(Rfc3501Tokenizer* tokenizer, SemanticActions* semantic);
	virtual ~ImapParser();
	
	enum ParseResult {
		PARSE_ERROR,
		PARSE_DONE,
		PARSE_NEEDS_DATA
	};

	/**
	 * Run parser.
	 * 
	 * Returns true if parsing completed successfully, false if more data is needed.
	 */
	ParseResult Parse();
	
protected:
	ParseResult ParseStep();
	void ParseError(Production* prod, int term, Symbol* symbol, const char* filename, int line);
	TokenType GetTokenType() const;
	
	static void initialize();
	static void BuildTokenTypeMap();
	
	Rfc3501Tokenizer*	m_tokenizer;
	SemanticActions*	m_semantic;
	
	std::stack<CallStackEntry> m_callStack;
	
	static bool s_initialized;
	static const std::locale& s_classicLocale;
	
	static Symbol* symbols[];
	static std::map<std::string, TokenType> s_tokenMap;
	static std::vector< std::pair<std::string, TokenType> > s_tokenList;
};

class CallStackEntry {
public:
	CallStackEntry(Production* p, int pos) :
		m_production(p), m_pos(pos) {
	}
	
	Production*	GetProduction() const { return m_production; }
	int			GetCurrentTerm() const { return m_pos; }
	
	void		NextTerm() { ++m_pos; }
	
protected:
	Production* m_production;
	int m_pos;
};

#endif /*IMAPPARSER_H_*/
