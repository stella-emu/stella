#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
	int val;
	Expression *exp;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	NUMBER	257
# define	LOG_OR	258
# define	LOG_AND	259
# define	LOG_NOT	260
# define	SHR	261
# define	SHL	262
# define	GTE	263
# define	LTE	264
# define	NE	265
# define	EQ	266
# define	UMINUS	267


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
