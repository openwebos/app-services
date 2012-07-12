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

public class Symbol {

	public String value;
	public boolean onLeft = false;
	public boolean onRight = false;
	public int index = -1;
	public HashSet<Symbol> first = new HashSet<Symbol>();
	public HashSet<Symbol> follow = new HashSet<Symbol>();
	public ArrayList<Production> productions = new ArrayList<Production>();
	
	public Symbol(String token) {
		value = token;
	}
	
	public String toString() {
		return value;
	}

	public boolean isTerminal() {
		return !onLeft;
	}
	
	public boolean isGoal() {
		return onLeft && !onRight;
	}

	public String firstSet() {
		return printSet(first);
	}

	public String followSet() {
		return printSet(follow);
	}

	public void addProduction(Production p) {
		productions.add(p);
	}

	public static String printSet(HashSet<Symbol> first) {
		StringBuffer s = new StringBuffer();
		boolean b = false;
		for (Symbol t : first) {
			if (!b)
				s.append("[");
			else
				s.append(",");
			s.append(t.value);
			b = true;
		}
		if (!b)
			s.append("[");
		s.append("]");
		return s.toString();
	}

}

