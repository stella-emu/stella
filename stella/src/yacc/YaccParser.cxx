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
// $Id: YaccParser.cxx,v 1.2 2005-07-03 14:18:54 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

//#include "YaccParser.hxx"

#ifdef __cplusplus
extern "C" {
#endif

namespace YaccParser {
#include <stdio.h>
#include <ctype.h>

int result;
//#include "y.tab.h"
#include "y.tab.c"


int getResult() {
	return result;
}


const char *input, *c;

enum {
	ST_DEFAULT,
	ST_NUMBER,
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

#define is_operator(x) ( (x=='+' || x=='-' || x=='*' || \
                          x=='/' || x=='<' || x=='>' || \
                          x=='|' || x=='&' || x=='^' || \
                          x=='!' || x=='~' || x=='(' || \
                          x==')' || x=='=' ) )

int yylex() {
	char o, p;
	yylval = 0;
	while(*c != '\0') {
		//fprintf(stderr, "looking at %c, state %d\n", *c, state);
		switch(state) {
			case ST_SPACE:
				yylval = 0;
				if(isspace(*c)) {
					c++;
				} else if(isdigit(*c)) {
					state = ST_NUMBER;
				} else if(is_operator(*c)) {
					state = ST_OPERATOR;
				} else {
					state = ST_DEFAULT;
				}

				break;

			case ST_NUMBER:
				while(isdigit(*c)) {
					yylval *= 10;
					yylval += (*c++ - '0');
					//fprintf(stderr, "yylval==%d, *c==%c\n", yylval, *c);
				}
				state = ST_DEFAULT;
				return NUMBER;
/*
				if(isdigit(*c)) {
					yylval *= 10;
					yylval += (*c - '0');
					c++;
					//fprintf(stderr, "*c==0? %d\n", *c==0);
					if(*c == '\0') return NUMBER;
					break;
				} else {
					state = ST_DEFAULT;
					return NUMBER;
				}
*/


			case ST_OPERATOR:
				o = *c++;
				if(!*c) return o;
				if(isspace(*c)) {
					state = ST_SPACE;
					return o;
				} else if(isdigit(*c)) {
					state = ST_NUMBER;
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
				yylval = 0;
				if(isspace(*c)) {
					state = ST_SPACE;
				} else if(isdigit(*c)) {
					state = ST_NUMBER;
				} else if(is_operator(*c)) {
					state = ST_OPERATOR;
				} else {
					yylval = *c++;
					return yylval;
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

#ifdef __cplusplus
}
#endif
