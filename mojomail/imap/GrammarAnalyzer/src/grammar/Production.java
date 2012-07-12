// @@@LICENSE
//
//      Copyright (c) 2009-2010 Hewlett-Packard Development Company, L.P.
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

package grammar;

import java.util.ArrayList;
import java.util.HashSet;

public class Production {

	public Symbol goal;
	public int number;
	public ArrayList<Term> terms = new ArrayList<Term>();
	public HashSet<Symbol> first = new HashSet<Symbol>();
	public String semanticAction = null;
	//public HashSet<Symbol> follow = new HashSet<Symbol>();
	
	public Production(Symbol goal, int number) {
		this.goal = goal;
		this.number = number;
	}
	
	public void addTerm(Term t) {
		terms.add(t);
	}
	
	public boolean setSemanticAction(String action) {
		if (terms.size()==0) {
			if (semanticAction!=null)
				return false;
			semanticAction = action;
		}
		else {
			Term t = terms.get(terms.size()-1);
			if (t.semanticAction!=null)
				return false;
			t.semanticAction = action;
		}
		return true;
	}

	public String toString() {
		StringBuffer b = new StringBuffer();
		b.append(goal);
		b.append(" =");
		for (Term s : terms) {
			b.append(" ");
			b.append(s);
		}
		return b.toString();
	}
	
}
