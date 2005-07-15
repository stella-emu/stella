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
	char *equate;
	Expression *exp;
}

/* Terminals */
%token <val> NUMBER
%token <equate> EQUATE

/* Non-terminals */
%type <exp> expression

/* Operator associativity and precedence */
%left '-' '+'
%left '*' '/' '%'
%left LOG_OR
%left LOG_AND
%left LOG_NOT
%left '|' '^'
%left '&'
%left SHR SHL
%nonassoc '<' '>' GTE LTE NE EQ
%nonassoc DEREF
%nonassoc UMINUS


%%
statement:	expression { fprintf(stderr, "\ndone\n"); result.exp = $1; }
	;

expression:	expression '+' expression { fprintf(stderr, " +"); $$ = new PlusExpression($1, $3); }
	|	expression '-' expression { fprintf(stderr, " -"); $$ = new MinusExpression($1, $3); }
	|	expression '*' expression { fprintf(stderr, " *"); $$ = new MultExpression($1, $3); }
	|	expression '/' expression { fprintf(stderr, " /"); $$ = new DivExpression($1, $3); }
	|	expression '%' expression { fprintf(stderr, " %%"); $$ = new ModExpression($1, $3);  }
	|	expression '&' expression { fprintf(stderr, " &"); $$ = new BinAndExpression($1, $3); }
	|	expression '|' expression { fprintf(stderr, " |"); $$ = new BinOrExpression($1, $3); }
	|	expression '^' expression { fprintf(stderr, " ^"); $$ = new BinXorExpression($1, $3); }
	|	expression '<' expression { fprintf(stderr, " <"); $$ = new LessExpression($1, $3); }
	|	expression '>' expression { fprintf(stderr, " >"); $$ = new GreaterExpression($1, $3); }
	|	expression GTE expression { fprintf(stderr, " >="); $$ = new GreaterEqualsExpression($1, $3); }
	|	expression LTE expression { fprintf(stderr, " <="); $$ = new LessEqualsExpression($1, $3); }
	|	expression NE  expression { fprintf(stderr, " !="); $$ = new NotEqualsExpression($1, $3); }
	|	expression EQ  expression { fprintf(stderr, " =="); $$ = new EqualsExpression($1, $3); }
	|	expression SHR expression { fprintf(stderr, " >>"); $$ = new ShiftRightExpression($1, $3); }
	|	expression SHL expression { fprintf(stderr, " <<"); $$ = new ShiftLeftExpression($1, $3); }
	|	expression LOG_OR expression { fprintf(stderr, " ||"); $$ = new LogOrExpression($1, $3); }
	|	expression LOG_AND expression { fprintf(stderr, " &&"); $$ = new LogAndExpression($1, $3); }
	|	'-' expression %prec UMINUS	{ fprintf(stderr, " U-"); $$ = new UnaryMinusExpression($2); }
	|	'~' expression %prec UMINUS	{ fprintf(stderr, " ~"); $$ = new BinNotExpression($2); }
	|	'!' expression %prec UMINUS	{ fprintf(stderr, " !"); $$ = new LogNotExpression($2); }
	|	'*' expression %prec DEREF { fprintf(stderr, " U*"); $$ = new ByteDerefExpression($2); }
	|	'@' expression %prec DEREF { fprintf(stderr, " U@"); $$ = new WordDerefExpression($2); }
	|	'<' expression { fprintf(stderr, " U<");  $$ = new LoByteExpression($2);  }
	|	'>' expression { fprintf(stderr, " U>");  $$ = new HiByteExpression($2);  }
	|	'(' expression ')'	{ fprintf(stderr, " ()"); $$ = $2; }
	|	NUMBER { fprintf(stderr, " %d", $1); $$ = new ConstExpression($1); }
	|	EQUATE { fprintf(stderr, " %s", $1); $$ = new EquateExpression($1); }
	;
%%
