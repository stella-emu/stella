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
# define	SHR	260
# define	SHL	261
# define	GTE	262
# define	LTE	263
# define	NE	264
# define	EQ	265
# define	UMINUS	266


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
