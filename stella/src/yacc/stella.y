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
%nonassoc '<' '>' GTE LTE NE EQ
%nonassoc UMINUS

%%
statement:	expression { result = $1; }
	;

expression:	expression '+' expression { fprintf(stderr, " +"); $$ = $1 + $3; }
	|	expression '-' expression { fprintf(stderr, " -"); $$ = $1 - $3; }
	|	expression '*' expression { fprintf(stderr, " *"); $$ = $1 * $3; }
	|	expression '/' expression
				{	fprintf(stderr, " /");
					if($3 == 0)
						yyerror("divide by zero");
					else
						$$ = $1 / $3;
				}
	|	expression '%' expression
				{	fprintf(stderr, " %");
					if($3 == 0)
						yyerror("divide by zero");
					else
						$$ = $1 % $3;
				}
	|	expression '&' expression { fprintf(stderr, " &"); $$ = $1 & $3; }
	|	expression '|' expression { fprintf(stderr, " |"); $$ = $1 | $3; }
	|	expression '^' expression { fprintf(stderr, " ^"); $$ = $1 ^ $3; }
	|	expression '>' expression { fprintf(stderr, " <"); $$ = $1 > $3; }
	|	expression '<' expression { fprintf(stderr, " >"); $$ = $1 < $3; }
	|	expression GTE expression { fprintf(stderr, " >="); $$ = $1 >= $3; }
	|	expression LTE expression { fprintf(stderr, " <="); $$ = $1 <= $3; }
	|	expression NE  expression { fprintf(stderr, " !="); $$ = $1 != $3; }
	|	expression EQ  expression { fprintf(stderr, " =="); $$ = $1 == $3; }
	|	expression SHR expression { fprintf(stderr, " >>"); $$ = $1 >> $3; }
	|	expression SHL expression { fprintf(stderr, " >>"); $$ = $1 << $3; }
	|	expression LOG_OR expression { fprintf(stderr, " ||"); $$ = $1 || $3; }
	|	expression LOG_AND expression { fprintf(stderr, " &&"); $$ = $1 && $3; }
	|	'-' expression %prec UMINUS	{ fprintf(stderr, " U-"); $$ = -$2; }
	|	'~' expression %prec UMINUS	{ fprintf(stderr, " ~"); $$ = (~$2) & 0xffff; }
	|	'<' expression { fprintf(stderr, " <"); $$ = $2 & 0xff; }
	|	'>' expression { fprintf(stderr, " >"); $$ = ($2 >> 8) & 0xff; }
	|	'(' expression ')'	{ fprintf(stderr, " ()"); $$ = $2; }
	|	NUMBER { fprintf(stderr, " %d", $1); }
	;
%%
