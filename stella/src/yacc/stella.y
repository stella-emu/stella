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

%union {
	int val;
	Expression *exp;
}

%token <val> NUMBER
%left '-' '+'
%left '*' '/' '%'
%left LOG_OR
%left LOG_AND
%left '|' '^'
%left '&'
%left SHR SHL
%nonassoc '<' '>' GTE LTE NE EQ
%nonassoc UMINUS

%type <exp> expression

%%
statement:	expression { fprintf(stderr, "\ndone\n"); result.exp = $1; }
	;

expression:	expression '+' expression { fprintf(stderr, " +"); $$ = new PlusExpression($1, $3); }
	|	expression '-' expression { fprintf(stderr, " -"); $$ = new MinusExpression($1, $3); }
	|	expression '*' expression { fprintf(stderr, " *");  }
	|	expression '/' expression
				{	fprintf(stderr, " /");
/*
					if($3 == 0)
						yyerror("divide by zero");
					else
						$$ = $1 / $3;
*/
				}
	|	expression '%' expression
				{	fprintf(stderr, " %");
/*
					if($3 == 0)
						yyerror("divide by zero");
					else
						$$ = $1 % $3;
*/
				}
	|	expression '&' expression { fprintf(stderr, " &"); }
	|	expression '|' expression { fprintf(stderr, " |"); }
	|	expression '^' expression { fprintf(stderr, " ^"); }
	|	expression '>' expression { fprintf(stderr, " <"); }
	|	expression '<' expression { fprintf(stderr, " >"); }
	|	expression GTE expression { fprintf(stderr, " >="); }
	|	expression LTE expression { fprintf(stderr, " <="); }
	|	expression NE  expression { fprintf(stderr, " !="); }
	|	expression EQ  expression { fprintf(stderr, " =="); $$ = new EqualsExpression($1, $3); }
	|	expression SHR expression { fprintf(stderr, " >>"); }
	|	expression SHL expression { fprintf(stderr, " >>"); }
	|	expression LOG_OR expression { fprintf(stderr, " ||"); }
	|	expression LOG_AND expression { fprintf(stderr, " &&"); }
	|	'-' expression %prec UMINUS	{ fprintf(stderr, " U-"); }
	|	'~' expression %prec UMINUS	{ fprintf(stderr, " ~"); }
	|	'<' expression { fprintf(stderr, " <"); }
	|	'>' expression { fprintf(stderr, " >"); }
	|	'(' expression ')'	{ fprintf(stderr, " ()"); }
	|	NUMBER { fprintf(stderr, " %d", $1); $$ = new ConstExpression($1); }
	;
%%
