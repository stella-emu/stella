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
// Copyright (c) 1995-2026 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

// Bison C++ skeleton; requires Bison >= 3.2
%skeleton "lalr1.cc"
%require  "3.2"
%output   "stella.tab.cxx"
%defines  "stella.tab.hxx"

%define api.namespace    {YaccParser}
%define api.token.constructor
%define api.value.type   variant
%define parse.error      detailed
%define parse.assert

// Suppress warnings in the Bison-generated implementation file
%code top {
#ifndef _MSC_VER
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
#endif
}

// Types needed in the generated header (stella.tab.hxx)
%code requires {
// Suppress clang warnings in Bison-generated code
#ifdef __clang__
#  pragma clang system_header
#endif
// Suppress MSVC C4065 in Bison-generated code
#ifdef _MSC_VER
#  pragma warning(push)
#  pragma warning(disable: 4065)
#endif
  #include "bspf.hxx"
  #include "Expression.hxx"
  #include "CartDebug.hxx"
  #include "CpuDebug.hxx"
  #include "RiotDebug.hxx"
  #include "TIADebug.hxx"
  namespace YaccParser { class Lexer; }
}

%code provides {
#ifdef _MSC_VER
#  pragma warning(pop)
#endif
}

// Lexer reference passed to both the parser and yylex()
%param       { Lexer& lexer }
// Outputs accessible after parsing
%parse-param { unique_ptr<Expression>& result }
%parse-param { string& errorMsg }

// Code placed in stella.tab.cxx only (not the header)
%code {
  #include "DebuggerExpressions.hxx"
  namespace YaccParser {
    parser::symbol_type yylex(Lexer& lexer);
  }
}

// ---------------------------------------------------------------------------
// Token declarations
// ---------------------------------------------------------------------------

%token <int>        NUMBER
%token              ERR
%token <string>     EQUATE
%token <CartMethod> CART_METHOD
%token <CpuMethod>  CPU_METHOD
%token <RiotMethod> RIOT_METHOD
%token <TiaMethod>  TIA_METHOD
%token <string>     FUNCTION

// Multi-character operators (no semantic value)
%token LOG_OR LOG_AND SHR SHL GTE LTE NE EQ

// Pseudo-tokens used only as %prec targets
%token DEREF UMINUS

// ---------------------------------------------------------------------------
// Non-terminal types
// ---------------------------------------------------------------------------

%type <unique_ptr<Expression>> expression

// ---------------------------------------------------------------------------
// Operator precedence (lowest to highest)
// ---------------------------------------------------------------------------

%left  '+' '-'
%left  '*' '/' '%'
%left  LOG_OR
%left  LOG_AND
%left  '|' '^'
%left  '&'
%left  SHR SHL
%nonassoc '<' '>' GTE LTE NE EQ
%nonassoc DEREF
%nonassoc UMINUS
%nonassoc '['

%%

statement:
    expression  { result = std::move($1); }
  ;

expression:
    expression '+' expression  { $$ = std::make_unique<PlusExpression>         (std::move($1), std::move($3)); }
  | expression '-' expression  { $$ = std::make_unique<MinusExpression>        (std::move($1), std::move($3)); }
  | expression '*' expression  { $$ = std::make_unique<MultExpression>         (std::move($1), std::move($3)); }
  | expression '/' expression  { $$ = std::make_unique<DivExpression>          (std::move($1), std::move($3)); }
  | expression '%' expression  { $$ = std::make_unique<ModExpression>          (std::move($1), std::move($3)); }
  | expression '&' expression  { $$ = std::make_unique<BinAndExpression>       (std::move($1), std::move($3)); }
  | expression '|' expression  { $$ = std::make_unique<BinOrExpression>        (std::move($1), std::move($3)); }
  | expression '^' expression  { $$ = std::make_unique<BinXorExpression>       (std::move($1), std::move($3)); }
  | expression '<' expression  { $$ = std::make_unique<LessExpression>         (std::move($1), std::move($3)); }
  | expression '>' expression  { $$ = std::make_unique<GreaterExpression>      (std::move($1), std::move($3)); }
  | expression GTE expression  { $$ = std::make_unique<GreaterEqualsExpression>(std::move($1), std::move($3)); }
  | expression LTE expression  { $$ = std::make_unique<LessEqualsExpression>   (std::move($1), std::move($3)); }
  | expression NE  expression  { $$ = std::make_unique<NotEqualsExpression>    (std::move($1), std::move($3)); }
  | expression EQ  expression  { $$ = std::make_unique<EqualsExpression>       (std::move($1), std::move($3)); }
  | expression SHR expression  { $$ = std::make_unique<ShiftRightExpression>   (std::move($1), std::move($3)); }
  | expression SHL expression  { $$ = std::make_unique<ShiftLeftExpression>    (std::move($1), std::move($3)); }
  | expression LOG_OR  expression  { $$ = std::make_unique<LogOrExpression>  (std::move($1), std::move($3)); }
  | expression LOG_AND expression  { $$ = std::make_unique<LogAndExpression> (std::move($1), std::move($3)); }
  | '-' expression  %prec UMINUS   { $$ = std::make_unique<UnaryMinusExpression>(std::move($2)); }
  | '~' expression  %prec UMINUS   { $$ = std::make_unique<BinNotExpression>    (std::move($2)); }
  | '!' expression  %prec UMINUS   { $$ = std::make_unique<LogNotExpression>    (std::move($2)); }
  | '*' expression  %prec DEREF    { $$ = std::make_unique<ByteDerefExpression> (std::move($2)); }
  | '@' expression  %prec DEREF    { $$ = std::make_unique<WordDerefExpression> (std::move($2)); }
  | '<' expression                 { $$ = std::make_unique<LoByteExpression>    (std::move($2)); }
  | '>' expression                 { $$ = std::make_unique<HiByteExpression>    (std::move($2)); }
  | '(' expression ')'             { $$ = std::move($2); }
  | expression '[' expression ']'  { $$ = std::make_unique<ByteDerefOffsetExpression>(std::move($1), std::move($3)); }
  | NUMBER      { $$ = std::make_unique<ConstExpression>    ($1); }
  | EQUATE      { $$ = std::make_unique<EquateExpression>   ($1); }
  | CPU_METHOD  { $$ = std::make_unique<CpuMethodExpression> ($1); }
  | CART_METHOD { $$ = std::make_unique<CartMethodExpression>($1); }
  | RIOT_METHOD { $$ = std::make_unique<RiotMethodExpression>($1); }
  | TIA_METHOD  { $$ = std::make_unique<TiaMethodExpression> ($1); }
  | FUNCTION    { $$ = std::make_unique<FunctionExpression>  ($1); }
  | ERR         { error("Invalid label or constant"); YYERROR; }
  ;

%%

void YaccParser::parser::error(const string& msg)
{
  errorMsg = msg;
}
