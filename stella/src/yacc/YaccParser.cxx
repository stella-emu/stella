//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: YaccParser.cxx,v 1.17 2005-07-19 01:31:37 urchlay Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

//#include "YaccParser.hxx"

//#ifdef __cplusplus
//extern "C" {
//#endif

#include "Expression.hxx"
#include "CpuDebug.hxx"
#include "TIADebug.hxx"

#include "BinAndExpression.hxx"
#include "BinNotExpression.hxx"
#include "BinOrExpression.hxx"
#include "BinXorExpression.hxx"
#include "CpuMethodExpression.hxx"
#include "TiaMethodExpression.hxx"
#include "ByteDerefExpression.hxx"
#include "ByteDerefOffsetExpression.hxx"
#include "WordDerefExpression.hxx"
#include "ConstExpression.hxx"
#include "DivExpression.hxx"
#include "EqualsExpression.hxx"
#include "EquateExpression.hxx"
#include "Expression.hxx"
#include "GreaterEqualsExpression.hxx"
#include "GreaterExpression.hxx"
#include "HiByteExpression.hxx"
#include "LoByteExpression.hxx"
#include "LessEqualsExpression.hxx"
#include "LessExpression.hxx"
#include "LogAndExpression.hxx"
#include "LogOrExpression.hxx"
#include "LogNotExpression.hxx"
#include "MinusExpression.hxx"
#include "ModExpression.hxx"
#include "MultExpression.hxx"
#include "NotEqualsExpression.hxx"
#include "PlusExpression.hxx"
#include "ShiftLeftExpression.hxx"
#include "ShiftRightExpression.hxx"
#include "UnaryMinusExpression.hxx"

namespace YaccParser {
#include <stdio.h>
#include <ctype.h>

#include "y.tab.h"
yystype result;
string errMsg;
#include "y.tab.c"

const string& errorMessage() {
	return errMsg;
}

Expression *getResult() {
	return result.exp;
}


const char *input, *c;

enum {
	ST_DEFAULT,
	ST_IDENTIFIER,
	ST_OPERATOR,
	ST_SPACE
};

int state = ST_DEFAULT;

//extern int yylval; // bison provides this

void setInput(const char *in) {
	input = c = in;
	state = ST_DEFAULT;
}

int parse(const char *in) {
	errMsg = "(no error)";
	setInput(in);
	return yyparse();
}

/* hand-rolled lexer. Hopefully faster than flex... */

inline bool is_base_prefix(char x) { return ( (x=='\\' || x=='$' || x=='#') ); }

inline bool is_identifier(char x) {
	return ( (x>='0' && x<='9') || 
		      (x>='a' && x<='z') ||
		      (x>='A' && x<='Z') ||
		       x=='.' || x=='_'  );
}


inline bool is_operator(char x) {
	return ( (x=='+' || x=='-' || x=='*' ||
             x=='/' || x=='<' || x=='>' ||
             x=='|' || x=='&' || x=='^' ||
             x=='!' || x=='~' || x=='(' ||
             x==')' || x=='=' || x=='%' ||
				 x=='[' || x==']' ) );
}

// const_to_int converts a string into a number, in either the
// current base, or (if there's a base override) the selected base.
// Returns -1 on error, since negative numbers are the parser's
// responsibility, not the lexer's
int const_to_int(char *c) {
	// what base is the input in?
	BaseFormat base = Debugger::debugger().parser()->base();

	switch(*c) {
		case '\\':
			base = kBASE_2;
			c++;
			break;

		case '#':
			base = kBASE_10;
			c++;
			break;

		case '$':
			base = kBASE_16;
			c++;
			break;

		default: // not a base_prefix, use default base
			break;
	}

	int ret = 0;
	switch(base) {
		case kBASE_2:
			while(*c) {
				if(*c != '0' && *c != '1')
					return -1;
				ret *= 2;
				ret += (*c - '0');
				c++;
			}
			return ret;

		case kBASE_10:
			while(*c) {
				if(!isdigit(*c))
					return -1;
				ret *= 10;
				ret += (*c - '0');
				c++;
			}
			return ret;

		case kBASE_16:
			while(*c) { // FIXME: error check!
				if(!isxdigit(*c))
					return -1;
				int dig = (*c - '0');
				if(dig > 9) dig = tolower(*c) - 'a' + 10;
				ret *= 16;
				ret += dig;
				c++;
			}
			return ret;

		default:
			fprintf(stderr, "INVALID BASE in lexer!");
			return 0;
	}
}

// special methods that get e.g. CPU registers
// TODO: store in a map or something
CPUDEBUG_INT_METHOD getCpuSpecial(char *c) {
	if(strcmp(c, "a") == 0)
		return &CpuDebug::a;

	if(strcmp(c, "x") == 0)
		return &CpuDebug::x;

	if(strcmp(c, "y") == 0)
		return &CpuDebug::y;

	if(strcmp(c, "pc") == 0)
		return &CpuDebug::pc;

	if(strcmp(c, "sp") == 0)
		return &CpuDebug::sp;

	if(strcmp(c, "c") == 0)
		return &CpuDebug::c;

	if(strcmp(c, "z") == 0)
		return &CpuDebug::z;

	if(strcmp(c, "n") == 0)
		return &CpuDebug::n;

	if(strcmp(c, "v") == 0)
		return &CpuDebug::v;

	if(strcmp(c, "d") == 0)
		return &CpuDebug::d;

	if(strcmp(c, "i") == 0)
		return &CpuDebug::i;

	if(strcmp(c, "b") == 0)
		return &CpuDebug::b;

	if(strcmp(c, "_bank") == 0)
		return &CpuDebug::getBank;

	return 0;
}

// special methods that get TIA internal state
TIADEBUG_INT_METHOD getTiaSpecial(char *c) {
	if(strcmp(c, "_scan") == 0)
		return &TIADebug::scanlines;

	return 0;
}

int yylex() {
	static char idbuf[255];
	char o, p;
	yylval.val = 0;
	while(*c != '\0') {
		//fprintf(stderr, "looking at %c, state %d\n", *c, state);
		switch(state) {
			case ST_SPACE:
				yylval.val = 0;
				if(isspace(*c)) {
					c++;
				} else if(is_identifier(*c) || is_base_prefix(*c)) {
					state = ST_IDENTIFIER;
				} else if(is_operator(*c)) {
					state = ST_OPERATOR;
				} else {
					state = ST_DEFAULT;
				}

				break;

			case ST_IDENTIFIER:
				{
					CPUDEBUG_INT_METHOD cpuMeth;
					TIADEBUG_INT_METHOD tiaMeth;

					char *bufp = idbuf;
					*bufp++ = *c++; // might be a base prefix
					while(is_identifier(*c)) { // may NOT be base prefixes
						*bufp++ = *c++;
						//fprintf(stderr, "yylval==%d, *c==%c\n", yylval, *c);
					}
					*bufp = '\0';
					state = ST_DEFAULT;

					// Note: specials (like "a" for accumulator) have priority over
					// numbers. So "a" always means accumulator, not hex 0xa. User
					// is welcome to use a base prefix ("$a"), or a capital "A",
					// to mean 0xa.

					// Also, labels have priority over specials, so Bad Things will
					// happen if the user defines a label that matches one of
					// the specials. Who would do that, though?

					if(Debugger::debugger().equates()->getAddress(idbuf) > -1) {
						yylval.equate = idbuf;
						return EQUATE;
					} else if( (cpuMeth = getCpuSpecial(idbuf)) ) {
						yylval.cpuMethod = cpuMeth;
						return CPU_METHOD;
					} else if( (tiaMeth = getTiaSpecial(idbuf)) ) {
						yylval.tiaMethod = tiaMeth;
						return TIA_METHOD;
					} else {
						yylval.val = const_to_int(idbuf);
						if(yylval.val >= 0)
							return NUMBER;
						else
							return ERR;
					}
				}

			case ST_OPERATOR:
				o = *c++;
				if(!*c) return o;
				if(isspace(*c)) {
					state = ST_SPACE;
					return o;
				} else if(is_identifier(*c) || is_base_prefix(*c)) {
					state = ST_IDENTIFIER;
					return o;
				} else {
					state = ST_DEFAULT;
					p = *c++;
					//fprintf(stderr, "o==%c, p==%c\n", o, p);
			 		if(o == '>' && p == '=')
						return GTE;
					else if(o == '<' && p == '=')
						return LTE;
					else if(o == '!' && p == '=')
						return NE;
					else if(o == '=' && p == '=')
						return EQ;
					else if(o == '|' && p == '|')
						return LOG_OR;
					else if(o == '&' && p == '&')
						return LOG_AND;
					else if(o == '<' && p == '<')
						return SHL;
					else if(o == '>' && p == '>')
						return SHR;
					else {
						c--;
						return o;
					}
				}

				break;

			case ST_DEFAULT:
			default:
				yylval.val = 0;
				if(isspace(*c)) {
					state = ST_SPACE;
				} else if(is_identifier(*c) || is_base_prefix(*c)) {
					state = ST_IDENTIFIER;
				} else if(is_operator(*c)) {
					state = ST_OPERATOR;
				} else {
					yylval.val = *c++;
					return yylval.val;
				}
				break;
		}
	}

	//fprintf(stderr, "end of input\n");
	return 0; // hit NUL, end of input.
}


#if 0
int main(int argc, char **argv) {
	int l;

	set_input(argv[1]);
	while( (l = yylex()) != 0 )
		printf("ret %d, %d\n", l, yylval);

	printf("%d\n", yylval);
}
#endif
}

//#ifdef __cplusplus
//}
//#endif
