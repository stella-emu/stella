%{
#include <stdio.h>

Expression* lastExp = 0;

#define YYERROR_VERBOSE 1

int yylex();
char *yytext;

void yyerror(char *e) {
	//fprintf(stderr, "%s at token \"%s\"\n", e, yytext);
	fprintf(stderr, "%s\n", e);
	errMsg = e;
	if(lastExp)
		delete lastExp;
}

%}

%union {
	int val;
	char *equate;
	CPUDEBUG_INT_METHOD intMethod;
	Expression *exp;
}

/* Terminals */
%token <val> NUMBER
%token <val> ERR
%token <equate> EQUATE
%token <intMethod> INT_METHOD

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

expression:	expression '+' expression { fprintf(stderr, " +"); $$ = new PlusExpression($1, $3); lastExp = $$; }
	|	expression '-' expression { fprintf(stderr, " -"); $$ = new MinusExpression($1, $3); lastExp = $$; }
	|	expression '*' expression { fprintf(stderr, " *"); $$ = new MultExpression($1, $3); lastExp = $$; }
	|	expression '/' expression { fprintf(stderr, " /"); $$ = new DivExpression($1, $3); lastExp = $$; }
	|	expression '%' expression { fprintf(stderr, " %%"); $$ = new ModExpression($1, $3);  lastExp = $$; }
	|	expression '&' expression { fprintf(stderr, " &"); $$ = new BinAndExpression($1, $3); lastExp = $$; }
	|	expression '|' expression { fprintf(stderr, " |"); $$ = new BinOrExpression($1, $3); lastExp = $$; }
	|	expression '^' expression { fprintf(stderr, " ^"); $$ = new BinXorExpression($1, $3); lastExp = $$; }
	|	expression '<' expression { fprintf(stderr, " <"); $$ = new LessExpression($1, $3); lastExp = $$; }
	|	expression '>' expression { fprintf(stderr, " >"); $$ = new GreaterExpression($1, $3); lastExp = $$; }
	|	expression GTE expression { fprintf(stderr, " >="); $$ = new GreaterEqualsExpression($1, $3); lastExp = $$; }
	|	expression LTE expression { fprintf(stderr, " <="); $$ = new LessEqualsExpression($1, $3); lastExp = $$; }
	|	expression NE  expression { fprintf(stderr, " !="); $$ = new NotEqualsExpression($1, $3); lastExp = $$; }
	|	expression EQ  expression { fprintf(stderr, " =="); $$ = new EqualsExpression($1, $3); lastExp = $$; }
	|	expression SHR expression { fprintf(stderr, " >>"); $$ = new ShiftRightExpression($1, $3); lastExp = $$; }
	|	expression SHL expression { fprintf(stderr, " <<"); $$ = new ShiftLeftExpression($1, $3); lastExp = $$; }
	|	expression LOG_OR expression { fprintf(stderr, " ||"); $$ = new LogOrExpression($1, $3); lastExp = $$; }
	|	expression LOG_AND expression { fprintf(stderr, " &&"); $$ = new LogAndExpression($1, $3); lastExp = $$; }
	|	'-' expression %prec UMINUS	{ fprintf(stderr, " U-"); $$ = new UnaryMinusExpression($2); lastExp = $$; }
	|	'~' expression %prec UMINUS	{ fprintf(stderr, " ~"); $$ = new BinNotExpression($2); lastExp = $$; }
	|	'!' expression %prec UMINUS	{ fprintf(stderr, " !"); $$ = new LogNotExpression($2); lastExp = $$; }
	|	'*' expression %prec DEREF { fprintf(stderr, " U*"); $$ = new ByteDerefExpression($2); lastExp = $$; }
	|	'@' expression %prec DEREF { fprintf(stderr, " U@"); $$ = new WordDerefExpression($2); lastExp = $$; }
	|	'<' expression { fprintf(stderr, " U<");  $$ = new LoByteExpression($2);  lastExp = $$; }
	|	'>' expression { fprintf(stderr, " U>");  $$ = new HiByteExpression($2);  lastExp = $$; }
	|	'(' expression ')'	{ fprintf(stderr, " ()"); $$ = $2; lastExp = $$; }
	|	NUMBER { fprintf(stderr, " %d", $1); $$ = new ConstExpression($1); lastExp = $$; }
	|	EQUATE { fprintf(stderr, " %s", $1); $$ = new EquateExpression($1); lastExp = $$; }
	|	INT_METHOD { fprintf(stderr, " (intMethod)"); $$ = new IntMethodExpression($1); lastExp = $$; }
	|  ERR { fprintf(stderr, " ERR"); yyerror("Invalid label or constant"); return 1; }
	;
%%
