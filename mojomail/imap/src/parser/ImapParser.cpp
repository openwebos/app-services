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

#include "parser/ImapParser.h"
#include "parser/ParserTypes.h"
#include "parser/SemanticActions.h"
#include "parser/Rfc3501Tokenizer.h"
#include "exceptions/Rfc3501ParseException.h"
#include <sstream>
#include <cctype>
#include <boost/algorithm/string/predicate.hpp>

using namespace std;

map<string, TokenType> ImapParser::s_tokenMap;
vector< std::pair<string, TokenType> > ImapParser::s_tokenList;

bool ImapParser::s_initialized = false;

const std::locale& ImapParser::s_classicLocale = std::locale::classic();

ImapParser::ImapParser(Rfc3501Tokenizer* tokenizer, SemanticActions* semantic)
: m_tokenizer(tokenizer), m_semantic(semantic)
{
	if(!s_initialized) {
		initialize();
		BuildTokenTypeMap();
		s_initialized = true;
	}
	
	Symbol* sym = symbols[TK_goal];
	tokenizer->next();
	int tt = GetTokenType();
	
	Production* newp = sym->map[tt];
	if (newp == NULL)
		newp = sym->defaultProduction;
	if (newp == NULL) {
		ParseError(NULL, -1, sym, __FILE__, __LINE__);
	}

	m_callStack.push(CallStackEntry(newp, 0));
}

ImapParser::~ImapParser()
{
}

void ImapParser::ParseError(Production* prod, int term, Symbol* symbol, const char* filename, int line)
{
	string error = "error parsing";
	if(symbol) {
		stringstream ss;
		ss << "unexpected token " << GetTokenType() << " while parsing symbol " << symbol->str;
		if(prod) {
			ss << " in production " << prod->GetNumber() << " term " << term;
		}
		error = ss.str();
	}
	
	fprintf(stderr, "consumed: [%s]\n", m_tokenizer->getConsumedText().c_str());
	fprintf(stderr, "remaining: [%s]\n", m_tokenizer->getRemainingText().c_str());
	
	throw Rfc3501ParseException(error.c_str(), filename, line);
}

ImapParser::ParseResult ImapParser::Parse()
{
	return ParseStep();
}

void ImapParser::BuildTokenTypeMap()
{
	s_tokenMap["FETCH"] = TK_FETCH;
	s_tokenMap["FLAGS"] = TK_FLAGS;
	s_tokenMap["INTERNALDATE"] = TK_INTERNALDATE;
	s_tokenMap["UID"] = TK_UID;
	s_tokenMap["NIL"] = TK_NIL;
	s_tokenMap["BODY"] = TK_BODY;
	s_tokenMap["BODYSTRUCTURE"] = TK_BODYSTRUCTURE;
	s_tokenMap["ENVELOPE"] = TK_ENVELOPE;
	s_tokenMap["HEADER"] = TK_HEADER;
	s_tokenMap["HEADER.FIELDS"] = TK_HEADER_DOT_FIELDS;
	s_tokenMap["HEADER.FIELDS.NOT"] = TK_HEADER_DOT_FIELDS_DOT_NOT;
	s_tokenMap["RFC822.HEADER"] = TK_RFC822_DOT_HEADER;
	s_tokenMap["RFC822.SIZE"] = TK_RFC822_DOT_SIZE;
	s_tokenMap["RFC822.TEXT"] = TK_RFC822_DOT_TEXT;
	s_tokenMap["MIME"] = TK_MIME;
	s_tokenMap["EXPUNGE"] = TK_EXPUNGE;

	// FIXME use a hashmap
	s_tokenList.assign(s_tokenMap.begin(), s_tokenMap.end());

	// "TEXT", "RFC822", and "MESSAGE" are handled in GetTokenType since they're quoted strings
}

TokenType ImapParser::GetTokenType() const
{
	// FIXME: eventually remove this double layer or translation
	Rfc3501Tokenizer& t(*m_tokenizer);

	switch (t.tokenType) {
	case TK_LITERAL_BYTES:
		return TK_LITERAL_BYTES;
	case TK_TEXT:
	{
		// FIXME: replace Tokenizer
		// The exact mechanism for promoting these to tokens needs further thought.
		// We either need to change the grammar, or we need to add some semantic
		// actions to trigger the promotion.

		const std::string& token = t.value();

		if (token[0] >= '0' && token[0]<='9')
			return TK_NUMBER;

		if(true) {
			vector< std::pair<string, TokenType> >::iterator it;
			for(it = s_tokenList.begin(); it != s_tokenList.end(); ++it) {
				if( it->first.length() == token.length() && boost::iequals(it->first, token, s_classicLocale) )
					return it->second;
			}
		}

		//if (s_tokenMap[t.valueUpper()])
		//	return s_tokenMap[t.valueUpper()];
		return TK_ATOM;
	}
	case TK_QUOTED_STRING:
	{
		if ( boost::iequals(t.value(), "TEXT", s_classicLocale) )
			return TK_TEXT_IN_QUOTES;
		if ( boost::iequals(t.value(), "RFC822", s_classicLocale) )
			return TK_RFC822_IN_QUOTES;
		if ( boost::iequals(t.value(), "MESSAGE", s_classicLocale) )
			return TK_MESSAGE_IN_QUOTES;
		return TK_QUOTED_STRING;
	}
	default:
		return t.tokenType;
	}
	/* TODO
			, &TT_RFC822_DOT_TEXT
			, &TT_RFC822_DOT_SIZE
			, &TT_RFC822_DOT_HEADER
			, &TT_RFC822
			, &TT_MIME
			, &TT_HEADER_DOT_FIELDS_DOT_NOT
			, &TT_HEADER_DOT_FIELDS
			, &TT_EXPUNGE
	 */
}

ImapParser::ParseResult ImapParser::ParseStep()
{
	int lastTerminalSymbol = TK_ERROR;

	//throw MailException("dummy", __FILE__, __LINE__);

	while(!m_callStack.empty()) {
		// Get the current symbol
		CallStackEntry* top	= &m_callStack.top();
		Production* production = top->GetProduction();
		
		if(top->GetCurrentTerm() < production->GetNumTerms()) {

			const Term* term = production->GetTerm(top->GetCurrentTerm());
			Symbol* symbol = term->symbol;

			if (symbol->isTerminal()) {
				int tt = GetTokenType();

				if(false) {
					fprintf(stderr, "term %d sym %s\n", top->GetCurrentTerm(), symbol->str.c_str());
					fprintf(stderr, "parsing token '%s' type %d\n", m_tokenizer->value().c_str(), tt);
				}

				// The current token should match the symbol's literal value
				// For example, TT_LPAREN should match "(", TT_FLAGS should match "FLAGS"
				if (tt != symbol->value) {
					if (tt == TK_SP && (lastTerminalSymbol == TK_SP || lastTerminalSymbol == TK_LPAREN)) {
						// Workaround for buggy servers (Lotus Notes Domino)
						// Skip illegal space if it occurs after another space or '('
						fprintf(stderr, "skipping illegal space while parsing production %d\n", production->GetNumber());
						m_tokenizer->next();
						continue;
					}

					ParseError(production, top->GetCurrentTerm(), symbol, __FILE__, __LINE__);
				}

				// Run any associated semantic action
				if (term->func) {
					(m_semantic->* (term->func))();
				}

				// Save the last terminal symbol type
				lastTerminalSymbol = symbol->value;

				// Get the next token
				TokenType tokenType = m_tokenizer->next();

				// Advance to the next term in the production
				top->NextTerm();

				if(tokenType == TK_ERROR || tokenType == TK_QUOTED_STRING_WITHOUT_TERMINATION) {
					fprintf(stderr, "no more tokens available to satisfy production %d; returning from parser\n", production->GetNumber());
					//fprintf(stderr, "consumed: [%s]\n", m_tokenizer->getConsumedText().c_str());
					//fprintf(stderr, "remaining: [%s]\n", m_tokenizer->getRemainingText().c_str());

					return PARSE_NEEDS_DATA;
				}

			} else { // non-terminal
				int tt = GetTokenType();
				production = symbol->map[tt];

				if (production == NULL)
					production = symbol->defaultProduction;

				if (production == NULL) {
					if (tt == TK_SP && (lastTerminalSymbol == TK_SP || lastTerminalSymbol == TK_LPAREN)) {
						// Workaround for buggy servers (Lotus Notes Domino)
						// Skip illegal space if it occurs after another space or '('
						fprintf(stderr, "skipping illegal space while parsing non-terminal symbol %s\n", symbol->str.c_str());
						m_tokenizer->next();
						continue;
					}

					ParseError(NULL, top->GetCurrentTerm(), symbol, __FILE__, __LINE__);
				}

				if(false) {
					fprintf(stderr, "pushing production %d on call stack; terms:", production->GetNumber());
					for(int i = 0; i < production->GetNumTerms(); i++) {
						fprintf(stderr, " %s", production->GetTerm(i)->symbol->str.c_str());
					}
					fprintf(stderr, "\n");
				}

				m_callStack.push(CallStackEntry(production, 0));

				// Run semantic actions associated with pushing the production
				if (production->func) {
					(m_semantic->* (production->func))();
				}
			}
		}
		
		if (!m_callStack.empty()) {
			top = &m_callStack.top();
			production = top->GetProduction();
			
			while (top->GetCurrentTerm() >= production->GetNumTerms()) {
				// No more terms in this production; pop it
				m_callStack.pop();
				
				if(false) {
					fprintf(stderr, "popping production %d from call stack", production->GetNumber());

					if(!m_callStack.empty()) {
						top = &m_callStack.top();
						production = top->GetProduction();

						if(top->GetCurrentTerm() < production->GetNumTerms()) {
							fprintf(stderr, "; remaining terms in %d:", production->GetNumber());

							for(int i = top->GetCurrentTerm(); i < production->GetNumTerms(); i++) {
								fprintf(stderr, " %s", production->GetTerm(i)->symbol->str.c_str());
							}
						}
					}
					fprintf(stderr, "\n");
				}

				if (m_callStack.empty())
					break;
				
				top = &m_callStack.top();

				// Run semantic action for non-terminal production
				production = top->GetProduction();

				if(top->GetCurrentTerm() < production->GetNumTerms()) {
					const Term* term = production->GetTerm( top->GetCurrentTerm() );
					if (term->func) {
						(m_semantic->* (term->func))();
					}

					top->NextTerm();
				} else {
					// we're in trouble
					stringstream error;
					error << "term " << top->GetCurrentTerm() << "/" <<
							production->GetNumTerms() << " out of bounds for production " << production->GetNumber();

					throw Rfc3501ParseException(error.str().c_str(), __FILE__, __LINE__);
				}
			}
		}
	}
	
	return PARSE_DONE;
}
