#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
	int val;
	char *equate;
	Expression *exp;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	NUMBER	257
# define	EQUATE	258
# define	LOG_OR	259
# define	LOG_AND	260
# define	LOG_NOT	261
# define	SHR	262
# define	SHL	263
# define	GTE	264
# define	LTE	265
# define	NE	266
# define	EQ	267
# define	DEREF	268
# define	UMINUS	269


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
