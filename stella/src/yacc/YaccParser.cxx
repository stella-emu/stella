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
// $Id: YaccParser.cxx,v 1.5 2005-07-14 15:13:58 urchlay Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

//#include "YaccParser.hxx"

//#ifdef __cplusplus
//extern "C" {
//#endif

#include "Expression.hxx"

#include "BinAndExpression.hxx"
#include "BinNotExpression.hxx"
#include "BinOrExpression.hxx"
#include "BinXorExpression.hxx"
#include "ConstExpression.hxx"
#include "DivExpression.hxx"
#include "EqualsExpression.hxx"
#include "EquateExpression.hxx"
#include "Expression.hxx"
#include "GreaterEqualsExpression.hxx"
#include "GreaterExpression.hxx"
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
#include "y.tab.c"


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
	setInput(in);
	return yyparse();
}

/* hand-rolled lexer. Hopefully faster than flex... */

#define is_identifier(x) ( (x>='0' && x<='9') || \
		                     (x>='a' && x<='z') || \
		                     (x>='A' && x<='Z') || \
		                     (x=='.' && x<='_') )

#define is_operator(x) ( (x=='+' || x=='-' || x=='*' || \
                          x=='/' || x=='<' || x=='>' || \
                          x=='|' || x=='&' || x=='^' || \
                          x=='!' || x=='~' || x=='(' || \
                          x==')' || x=='=' ) )

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
				} else if(is_identifier(*c)) {
					state = ST_IDENTIFIER;
				} else if(is_operator(*c)) {
					state = ST_OPERATOR;
				} else {
					state = ST_DEFAULT;
				}

				break;

			case ST_IDENTIFIER:
				{
					char *bufp = idbuf;
					while(is_identifier(*c)) {
						*bufp++ = *c++;
						//fprintf(stderr, "yylval==%d, *c==%c\n", yylval, *c);
					}
					*bufp = '\0';
					state = ST_DEFAULT;

					if(Debugger::debugger().equates()->getAddress(idbuf) > -1) {
						yylval.equate = idbuf;
						return EQUATE;
					} else {
						yylval.val = atoi(idbuf);
						return NUMBER;
					}
				}

			case ST_OPERATOR:
				o = *c++;
				if(!*c) return o;
				if(isspace(*c)) {
					state = ST_SPACE;
					return o;
				} else if(is_identifier(*c)) {
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
				} else if(is_identifier(*c)) {
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
