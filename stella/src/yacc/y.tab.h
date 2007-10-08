/* A Bison parser, made by GNU Bison 2.3.  */

/* Skeleton interface for Bison's Yacc-like parsers in C

   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */

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

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     NUMBER = 258,
     ERR = 259,
     EQUATE = 260,
     CPU_METHOD = 261,
     TIA_METHOD = 262,
     FUNCTION = 263,
     LOG_OR = 264,
     LOG_AND = 265,
     LOG_NOT = 266,
     SHL = 267,
     SHR = 268,
     EQ = 269,
     NE = 270,
     LTE = 271,
     GTE = 272,
     DEREF = 273,
     UMINUS = 274
   };
#endif
/* Tokens.  */
#define NUMBER 258
#define ERR 259
#define EQUATE 260
#define CPU_METHOD 261
#define TIA_METHOD 262
#define FUNCTION 263
#define LOG_OR 264
#define LOG_AND 265
#define LOG_NOT 266
#define SHL 267
#define SHR 268
#define EQ 269
#define NE 270
#define LTE 271
#define GTE 272
#define DEREF 273
#define UMINUS 274




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
#line 29 "stella.y"
{
	int val;
	char *equate;
	CPUDEBUG_INT_METHOD cpuMethod;
	TIADEBUG_INT_METHOD tiaMethod;
	Expression *exp;
	char *function;
}
/* Line 1489 of yacc.c.  */
#line 96 "y.tab.h"
	YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;

