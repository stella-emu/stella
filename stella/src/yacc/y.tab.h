#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
	int val;
	char *equate;
	CPUDEBUG_INT_METHOD intMethod;
	Expression *exp;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	NUMBER	257
# define	EQUATE	258
# define	INT_METHOD	259
# define	LOG_OR	260
# define	LOG_AND	261
# define	LOG_NOT	262
# define	SHR	263
# define	SHL	264
# define	GTE	265
# define	LTE	266
# define	NE	267
# define	EQ	268
# define	DEREF	269
# define	UMINUS	270


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
