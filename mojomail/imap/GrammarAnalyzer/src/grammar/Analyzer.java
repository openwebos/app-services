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

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

public class Analyzer {

	// usage:
	// java -cp bin grammar.Analyzer -list foo.txt -out ../src/parser/Parser ../src/grammar/Grammar.txt 
	// mv ../src/parser/Parser.h ../inc/parser/Parser.h

	String outPath = null;
	String listName = null;
	String parserClass = "Parser";
	PrintWriter listOut;
	BufferedReader rdr;
	
	ArrayList<String> extraLines = new ArrayList<String>();
	
	ArrayList<String> words = new ArrayList<String>();

	HashMap<String,Symbol> symbols = new HashMap<String,Symbol>();
	Symbol epsilon;
	Symbol dollar;
	Symbol goal;
	Symbol errorToken;
	
	String token = null;
	
	boolean wantList = false;
	
	int productionCount = 0;
	int errors = 0;
	
	private ArrayList<Production> productions = new ArrayList<Production>();
	
	public Analyzer(String [] args) throws Exception {
		String filename = null;
		int pos = 0;
		while (pos<args.length) {
			String s = args[pos++];
			if (s.equals("-out")) {
				outPath = args[pos++];
				continue;
			}
			if (s.equals("-list")) {
				listName = args[pos++];
				continue;
			}
			if (s.startsWith("-")) {
				throw new Exception("Unknown argument: "+s);
			}
			if (filename == null) { 
				filename = s;
				continue;
			}
			throw new Exception("Unexpected argument: "+s);
		}
		if (outPath==null) {
			outPath = "parser";
		}
		if (filename == null) {
			filename = "src/Grammar.txt";
		}

		if (listName == null)
			listOut = new PrintWriter(new OutputStreamWriter(System.out));
		else
			listOut = new PrintWriter(new FileWriter(listName));

		rdr = new BufferedReader(new FileReader(filename));

		getToken();
	}
	
	public void getExtraLines() throws IOException {
		for (;;) {
			String line = rdr.readLine();
			if (line==null)
				break;
			if (line.trim().equals("endprefix"))
				break;
			extraLines.add(line);
		}
	}
	
	public String getToken() throws IOException {
		while (words==null || words.size()==0) {
			String line = rdr.readLine();
			if (wantList)
				listOut.println(line);
			if (line==null)
				return null;
			//line = line.trim();
			String [] awords = line.split("\\s+");
			for (String w : awords) {
				if (w.length()>0) {
					if (w.startsWith(";"))
						break;
					words.add(w);
				}
			}
		}
		token = words.get(0);
		words.remove(0);
		return token;
	}
	
	public void analyze() throws IOException, ParseError {
		epsilon = addSymbol("epsilon");
		goal = addSymbol("goal");
		dollar = addSymbol("$$$");
		errorToken = addSymbol("ERROR");
		errorToken.onRight = true;

		for (;;) {
			if (token.equals("prefix")) {
				getExtraLines();
				getToken();
				continue;
			}
			if (token.equals("token")) {
				getToken();
				addSymbol(token);
				getToken();
				continue;
			}
			if (token.equals("parser")) {
				getToken();
				parserClass = token;
				getToken();
				continue;
			}
			if (token.equals("production")) {
				getToken();
				parseProduction();
				continue;
			}
			if (token.equals("end")) {
				break;
			}
			throw newParseError("Expected 'end'");
		}
		rdr.close();
		
		for (Production p : productions) {
			p.goal.addProduction(p);
		}
		
		computeFirstSet();

		computeFollowSet();

		if (true) {
			listOut.println("\nProductions:");
			for (Production p : productions) {
				listOut.println(p.number + ": define "+ String.valueOf(p));
				listOut.println("    first = "+Symbol.printSet(p.first));
			}
		}
		
		listOut.println("\nGoals:");
		for (Symbol s : sorted(symbols.values())) {
			if (s.isGoal()) {
				listOut.println("goalsym: "+s);
			}
		}

		if (true) {
			listOut.println("\nNonTerminals:");
			int n = 0;
			for (Symbol s : sorted(symbols.values())) {
				if (!s.isTerminal()) {
					listOut.println("nonterminal: " + n + " " + s);
					n++;
				}
			}
		}

		if (true) {
			listOut.println("\nTerminals:");
	
			int n = 0;
			for (Symbol s : sorted(symbols.values())) {
				if (s.isTerminal() && s!=dollar && s!=epsilon) {
					listOut.println("terminal: "+ n + " " + s + " = " + tokenName(s));
					n++;
				}
			}
		}
			
		if (false) {
			for (Symbol s : sorted(symbols.values())) {
				if (!s.isTerminal()) {
					listOut.println("nonterminal: "+s + " First = "+ s.firstSet());
					listOut.println("  Follow = "+ s.followSet());
				}
			}
		}

		if (true) {
			printProductionSelection();
		}
		listOut.println("There were "+errors+" errors");
		listOut.flush();
		
		if (listName!=null) {
			listOut.close();
			System.out.println("There were "+errors+" errors");
		}
		
	}

	private String getGuardString(String s) {
		// String for include ifdef guard
		int ix = s.lastIndexOf('/');
		if (ix>0) {
			s = s.substring(ix+1);
		}
		s = s.toUpperCase().replace('.', '_');
		return "__" + s + "__";
	}
	
	private void printProductionSelection() {
		for (Symbol s : sorted(symbols.values())) {
			if (s.isTerminal())
				continue;
			
			listOut.println("\nProduction Selection for "+s);
			
			HashMap<Symbol,Production> selectionMap = new HashMap<Symbol,Production>();
			for (Production p : s.productions) {
				listOut.println(" Choose "+p.number+": "+p.toString());
				listOut.println("    when " + Symbol.printSet(p.first));
				if (p.first.size()==0) {
					errors++;
					listOut.println("Error "+errors+": first set is empty ");
				}
				for (Symbol ss : p.first) {
					if (selectionMap.containsKey(ss)) {
						errors++;
						listOut.println("Error "+errors+": symbol "+ss.value +" predicts "+p.number+": " + p.toString());
						Production pp = selectionMap.get(ss);
						listOut.println("          and "+ss.value +" predicts " + pp.number+": " + pp.toString());
					}
					else {
						selectionMap.put(ss, p);
					}
				}
			}
		}
	}
	

	private Collection<Symbol> indexSorted(Collection<Symbol> values) {
		ArrayList<Symbol> l = new ArrayList<Symbol>(values);
		Collections.sort(l, new Comparator<Symbol>() {

			@Override
			public int compare(Symbol arg0, Symbol arg1) {
				return arg0.index - arg1.index;
			} });
		return l;
	}

	private Collection<Symbol> sorted(Collection<Symbol> values) {
		ArrayList<Symbol> l = new ArrayList<Symbol>(values);
		Collections.sort(l, new Comparator<Symbol>() {

			@Override
			public int compare(Symbol arg0, Symbol arg1) {
				return arg0.value.compareTo(arg1.value);
			} });
		return l;
	}

	private void computeFirstSet() {
		boolean chg = true;
		while (chg) {
			chg = false;
			for (Symbol s : symbols.values()) {
				if (s.isTerminal()) {
					if (!s.first.contains(s)) {
						chg = true;
						s.first.add(s);
					}
				}
			}
			for (Production p : productions) {
				chg |= addToFirst(p.goal, p.terms);
			}
			for (Production p : productions) {
				chg |= addToFirst(p, p.terms);
			}
		}
	}

	private boolean addToFirst(Symbol goal, ArrayList<Term> terms) {
		boolean chg = false;
		if (terms.size()==0)
			return false;

		boolean have_first = false;
		for (Term t : terms) {
			Symbol s = t.value;
			for (Symbol f : s.first) {
				if (!goal.first.contains(f)) {
					goal.first.add(f);
					chg = true;
				}
			}
			if (!s.first.contains(epsilon)) {
				have_first = true;
				break;
			}
		}
		if (!have_first) {
			if (!goal.first.contains(epsilon)) {
				goal.first.add(epsilon);
				chg = true;
			}
		}
		return chg;
	}

	private boolean addToFirst(Production prod, ArrayList<Term> terms) {
		boolean chg = false;
		// 2. if there is a production X -> Y1Y2...Yk then aff first(Y1..Yk) to first(X)
		for (Term t : terms) {
			Symbol s = t.value;
			for (Symbol f : s.first) {
				if (!prod.first.contains(f)) {
					prod.first.add(f);
					chg = true;
				}
			}
			if (!s.first.contains(epsilon)) {
				break;
			}
		}
		return chg;
	}

	private void computeFollowSet() {
		boolean chg = true;
		while (chg) {
			chg = false;
			if (!goal.follow.contains(dollar)) {
				chg = true;
				goal.follow.add(dollar);
			}
			for (Production p : productions) {
				chg |= addToFollow(p);
			}
		}
	}

	private boolean addToFollow(Production p) {
		boolean chg = false;
		int sz = p.terms.size();
		for (int i=0;i<sz;i++) {
			Symbol b = p.terms.get(i).value;
			List<Term> rest = p.terms.subList(i+1, sz);
			chg |= addFollow(b, rest, p.goal.follow);
		}
		return chg;
	}

	private boolean addFollow(Symbol b, List<Term> rest,
			HashSet<Symbol> follow) {
		boolean chg = false;
		boolean have_follow = false;
		for (Term t : rest) {
			Symbol s = t.value;
			for (Symbol f : s.first) {
				if (f!=epsilon && !b.follow.contains(f)) {
					b.follow.add(f);
					chg = true;
				}
			}
			if (!s.first.contains(epsilon)) {
				have_follow = true;
				break;
			}
		}
		if (!have_follow) {
			for (Symbol f : follow) {
				if (!b.follow.contains(f)) {
					b.follow.add(f);
					chg = true;
				}
			}
		}
		return chg;
	}

	private void parseProduction() throws ParseError, IOException {
		Symbol goal = addSymbol(token);
		if (goal.onLeft) {
			errors++;
			listOut.println("Error "+errors+": "+goal+" is alread defined");
		}
		goal.onLeft = true;
		if (!getToken().equals("="))
			throw newParseError("= expected");
		getToken();
		parseRightHandSide(goal);
	}
	
	private void parseRightHandSide(Symbol goal) throws IOException {
		Production currentProduction = createProduction(goal);
		for (;;) {
			if (token.equals("end") || token.equals("production"))
					break;
			if (token.equals("{{")) {
				getToken();
				String semanticAction = getSemanticAction();
				if (!currentProduction.setSemanticAction(semanticAction)) {
					errors++;
					listOut.println("Error "+errors+": semantic action is already defined");
				}

				continue;
			}
			if (token.equals("/")) {
				currentProduction = createProduction(goal);
				getToken();
				continue;
			}
			Symbol s = addSymbol(token);
			s.onRight = true;
			currentProduction.addTerm(new Term(s));
			getToken();
		}
	}

	private String getSemanticAction() throws IOException {
		String action = "";
		while (token!=null) {
			 if (token.equals("}}")) {
				 getToken();
				 break;
			 }
			action = action + " " + token;
			getToken();
		}
		return action;
	}

	private Symbol addSymbol(String string) {
		Symbol s = symbols.get(string);
		if (s==null) {
			s = new Symbol(string);
			symbols.put(string, s);
			//listOut.println("New symbol: "+s);
		}
		return s;
	}

	private Production createProduction(Symbol goal) {
		Production p = new Production(goal, productionCount++);
		productions.add(p);
		return p;
	}

	private ParseError newParseError(String msg) {
		return new ParseError(msg);
	}

	private static final String dquote = "\"";
	
	public static String symbolName(Symbol s) {
		if (s.isTerminal())
			return nameWithPrefix("TT", s);
		else
			return nameWithPrefix("NT", s);
	}
	
	public static String tokenName(Symbol s) {
		return nameWithPrefix("TK", s);
	}

	public static String nameWithPrefix(String prefix, Symbol s) {
		String name = s.value;
		if (name.startsWith(dquote) && name.endsWith(dquote)) {
			name = name.substring(1, name.length()-1);
			if (name.length()==1) {
				int c = name.charAt(0);
				switch (c) {
				case '.':
					return prefix+"_PERIOD";
				case '\\':
					return prefix+"_BACKSLASH";
				case '<':
					return prefix+"_LT";
				case '>':
					return prefix+"_GT";
				case '[':
					return prefix+"_LBRACKET";
				case ']':
					return prefix+"_RBRACKET";
				case '{':
					return prefix+"_LBRACE";
				case '}':
					return prefix+"_RBRACE";
				case '(':
					return prefix+"_LPAREN";
				case ')':
					return prefix+"_RPAREN";
				}
				name = prefix+"_CHAR_" +Integer.toString(name.charAt(0));
			}
			else {
				name = prefix+"_"+name.replaceAll("\\-", "_").replaceAll("\\.", "_DOT_").replaceAll("\\\\", "NOT_");
			}
		}
		else {
			// normal token name
			name = prefix+"_"+name.replace('-', '_');
		}
		return name;
	}

	private void numberSymbols() {
		int n = 0;
		
		errorToken.index = 0;
		n++;
		
		for (Symbol s : sorted(symbols.values())) {
			if (s.isTerminal() && s!=dollar && s!=epsilon&&s.index<0) {
				s.index = n++;
			}
		}
		for (Symbol s : sorted(symbols.values())) {
			if (!s.isTerminal() && s!=dollar && s!=epsilon&&s.index<0) {
				s.index = n++;
			}
		}
		
	}
	
	private void makeParser() throws IOException {
		PrintWriter declarations = new PrintWriter(new BufferedWriter(new FileWriter(outPath+".h")));
		PrintWriter implementation  = new PrintWriter(new BufferedWriter(new FileWriter(outPath+".cpp")));

		String guard = getGuardString(outPath+".h");
		implementation.println("// Generated by Grammar Analyzer");
		for (String l : extraLines) {
			implementation.println(l);
		}

		declarations.println("// Generated by Grammar Analyzer");
		declarations.println("#ifndef "+guard);
		declarations.println("#define "+guard);
		declarations.println();
		
		numberSymbols();
		Collection<Symbol> sortedSymbols = indexSorted(symbols.values());

		if (true) {
			int n = 0;
			declarations.println("enum TokenType {");
			for (Symbol s : sortedSymbols) {
				if (s.index>=0) {
					declarations.println("\t"
							+ (n>0 ? ", " : "")
							+ tokenName(s));
					n++;
				}
			}
			declarations.println("};");
		}
		
		if (true) {
			implementation.println("\n\n//Symbols:");
			for (Symbol s : sortedSymbols) {
				if (s.index>=0) {
					implementation.format("static Symbol %s(\"%s\",%s,%s);\n",
							symbolName(s),
							symbolName(s),
							tokenName(s),
							s.isTerminal() ? "true" : "false");
				}
			}
		}


		implementation.println("\n\n//Productions:");
		
		for (Production p : productions) {
			Term first = null;
			implementation.format("static Term termset_%d[] = {\n",
					p.number);
			
			for (Term t : p.terms) {
				if (t.value == epsilon)
					continue;
				
				implementation.format("\t%sTerm(&%s, %s )\n",
					(first == null ? "  " : ", "),
					symbolName(t.value),
					(t.semanticAction==null ? "NULL" : t.semanticAction) );
				first = t;
			}
			implementation.format("\t};\n\n");
			
			implementation.format("static Production production_%d( %d, &%s, termset_%d, %d, %s);\n\n", 
					p.number, p.number,
					symbolName(p.goal),
					p.number,
					epsilonFreeCount(p.terms),
					(p.semanticAction==null ? "NULL" : p.semanticAction ) );
		}
		
		if (true) {
			int n = 0;
			implementation.println("\n\nSymbol* "+parserClass+"::symbols[] = {");
			for (Symbol s : sortedSymbols) {
				if (s.index>=0) {
					implementation.println("\t"
							+ (n>0 ? ", " : "")
							+ "&" + symbolName(s));
					n++;
				}
			}
			implementation.println("};");
		}

		implementation.println("\n\nvoid "+parserClass+"::initialize(void) {");
		for (Production p : productions) {
			Symbol goal = p.goal;
			for (Symbol f : p.first) {
					if (f == epsilon)
						implementation.format("\t%s.setDefault(&production_%d);\n", symbolName(goal), p.number); 
					else
						implementation.format("\t%s.setMap(&production_%d,&%s);\n", symbolName(goal), p.number, symbolName(f)); 
			}
		}
		implementation.println("}");
	
		declarations.println("#endif /* " + guard + " */");
		declarations.close();
		
		implementation.close();
	}


	private int epsilonFreeCount(ArrayList<Term> terms) {
		int sz = 0;
		for (Term t : terms)
			if (t.value != epsilon)
				sz++;
		return sz;
	}

	/**
	 * @param args
	 * @throws Exception
	 */
	public static void main(String[] args) throws Exception {
		Analyzer a = new Analyzer(args);
		a.analyze();
		a.makeParser();
	}

}
