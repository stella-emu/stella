%{
#include <stdio.h>

#define YYERROR_VERBOSE 1

int yylex();
char *yytext;

void yyerror(char *e) {
	//fprintf(stderr, "%s at token \"%s\"\n", e, yytext);
	fprintf(stderr, "%s\n", e);
}

%}

%token NUMBER
%left '-' '+'
%left '*' '/' '%'
%left LOG_OR
%left LOG_AND
%left '|' '^'
%left '&'
%left SHR SHL
%nonassoc UMINUS '<' '>' GTE LTE NE EQ

%%
statement:	expression { result = $1; }
	;

expression:	expression '+' expression { $$ = $1 + $3; }
	|	expression '-' expression { $$ = $1 - $3; }
	|	expression '*' expression { $$ = $1 * $3; }
	|	expression '/' expression
				{	if($3 == 0)
						yyerror("divide by zero");
					else
						$$ = $1 / $3;
				}
	|	expression '%' expression
				{	if($3 == 0)
						yyerror("divide by zero");
					else
						$$ = $1 % $3;
				}
	|	expression '&' expression { $$ = $1 & $3; }
	|	expression '|' expression { $$ = $1 | $3; }
	|	expression '^' expression { $$ = $1 ^ $3; }
	|	expression '>' expression { $$ = $1 > $3; }
	|	expression '<' expression { $$ = $1 < $3; }
	|	expression GTE expression { $$ = $1 >= $3; }
	|	expression LTE expression { $$ = $1 <= $3; }
	|	expression NE  expression { $$ = $1 != $3; }
	|	expression EQ  expression { $$ = $1 == $3; }
	|	expression SHR expression { $$ = $1 >> $3; }
	|	expression SHL expression { $$ = $1 << $3; }
	|	expression LOG_OR expression { $$ = $1 || $3; }
	|	expression LOG_AND expression { $$ = $1 && $3; }
	|	'-' expression %prec UMINUS	{ $$ = -$2; }
	|	'~' expression %prec UMINUS	{ $$ = (~$2) & 0xffff; }
	|	'<' expression %prec UMINUS	{ $$ = $2 & 0xff; }
	|	'>' expression %prec UMINUS	{ $$ = ($2 >> 8) & 0xff; }
	|	'(' expression ')'	{ $$ = $2; }
	|	NUMBER
	;
%%
