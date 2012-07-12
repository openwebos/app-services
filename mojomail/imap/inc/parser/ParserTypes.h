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

#ifndef PARSERTYPES_H_
#define PARSERTYPES_H_

#include "exceptions/Rfc3501ParseException.h"

class SemanticActions;

typedef void (SemanticActions::* SemanticFunction) (void);

class Production;

typedef std::map<int,Production*> ProductionMap;

class Symbol {
public:
	const std::string str;
	int value;
	bool terminal;
	ProductionMap map;
	Production* defaultProduction;

	Symbol(const std::string& s, int v, bool t) :
		str(s), value(v), terminal(t), defaultProduction(0) {
	}

	bool isTerminal() const {
		return terminal;
	}

	void setDefault(Production* p) {
		defaultProduction = p;
	}
	void setMap(Production* p, Symbol* s) {
		map[s->value] = p;
	}
};

class Term {
public:
	Symbol* symbol;
	SemanticFunction func;
	Term(Symbol* s, SemanticFunction f) : symbol(s), func(f) {

	}
};

class Production {
public:
	Production(int n, Symbol* left, const Term* t, int nt, SemanticFunction f) :
		m_number(n), m_leftPart(left), m_terms(t), m_numTerms(nt), func(f)
	{
	}
	
	int GetNumber() const { return m_number; }
	int GetNumTerms() const { return m_numTerms; }
	
	const Term* GetTerm(int num) const
	{
		if(num < m_numTerms) {
			return &m_terms[num];
		} else {
			assert(num < m_numTerms);
			throw Rfc3501ParseException("Production::GetTerm() out of range", __FILE__, __LINE__);
		}
	}

protected:
	int m_number;
	Symbol* m_leftPart;
	const Term* m_terms;
	int m_numTerms;
public:
	SemanticFunction func;
};

#endif /*PARSERTYPES_H_*/
