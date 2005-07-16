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
# define	ERR	258
# define	EQUATE	259
# define	INT_METHOD	260
# define	LOG_OR	261
# define	LOG_AND	262
# define	LOG_NOT	263
# define	SHR	264
# define	SHL	265
# define	GTE	266
# define	LTE	267
# define	NE	268
# define	EQ	269
# define	DEREF	270
# define	UMINUS	271


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
