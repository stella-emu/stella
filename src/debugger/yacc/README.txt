Makefile.yacc    - Not part of the regular Stella build!
YaccParser.cxx   - C++ wrapper for the generated parser; contains the Lexer class
YaccParser.hxx   - Public API: YaccParser::parse() and YaccParser::errorMessage()
calctest.cxx     - Not part of Stella! Used for standalone testing of the lexer/parser.
module.mk        - Used for the regular Stella build
stella.y         - Bison C++ grammar source for the expression parser
stella.tab.cxx   - Generated parser (from stella.y). NOT BUILT AUTOMATICALLY!
stella.tab.hxx   - Generated parser header (from stella.y). NOT BUILT AUTOMATICALLY!

The parser uses the Bison C++ skeleton (lalr1.cc), requiring Bison >= 3.2.

Even though they're generated, stella.tab.cxx and stella.tab.hxx are checked into
the repository so that people without a local copy of Bison can still compile Stella.

If you modify stella.y, you MUST run "make -f Makefile.yacc" in this directory.
This will regenerate stella.tab.cxx and stella.tab.hxx.  Do this before committing.

To test the lexer/parser without the rest of Stella, run:

  make -f Makefile.yacc calctest

and then pass an expression as its argument:

  ./calctest '2+2'
  = 4

To benchmark the lexer/parser, build with -DBM:

  g++ -DBM ... -o calctest calctest.cxx ...
