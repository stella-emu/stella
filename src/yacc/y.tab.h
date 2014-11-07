/* A Bison parser, made by GNU Bison 3.0.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2013 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    NUMBER = 258,
    ERR = 259,
    EQUATE = 260,
    CART_METHOD = 261,
    CPU_METHOD = 262,
    TIA_METHOD = 263,
    FUNCTION = 264,
    LOG_OR = 265,
    LOG_AND = 266,
    LOG_NOT = 267,
    SHR = 268,
    SHL = 269,
    GTE = 270,
    LTE = 271,
    NE = 272,
    EQ = 273,
    DEREF = 274,
    UMINUS = 275
  };
#endif
/* Tokens.  */
#define NUMBER 258
#define ERR 259
#define EQUATE 260
#define CART_METHOD 261
#define CPU_METHOD 262
#define TIA_METHOD 263
#define FUNCTION 264
#define LOG_OR 265
#define LOG_AND 266
#define LOG_NOT 267
#define SHR 268
#define SHL 269
#define GTE 270
#define LTE 271
#define NE 272
#define EQ 273
#define DEREF 274
#define UMINUS 275

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE YYSTYPE;
union YYSTYPE
{
#line 28 "stella.y" /* yacc.c:1909  */

	int val;
	char *equate;
	CARTDEBUG_INT_METHOD cartMethod;
	CPUDEBUG_INT_METHOD  cpuMethod;
	TIADEBUG_INT_METHOD  tiaMethod;
	Expression *exp;
	char *function;

#line 104 "y.tab.h" /* yacc.c:1909  */
};
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
