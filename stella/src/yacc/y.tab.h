#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
	int val;
	char *equate;
	CPUDEBUG_INT_METHOD cpuMethod;
	TIADEBUG_INT_METHOD tiaMethod;
	Expression *exp;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	NUMBER	257
# define	ERR	258
# define	EQUATE	259
# define	CPU_METHOD	260
# define	TIA_METHOD	261
# define	LOG_OR	262
# define	LOG_AND	263
# define	LOG_NOT	264
# define	SHR	265
# define	SHL	266
# define	GTE	267
# define	LTE	268
# define	NE	269
# define	EQ	270
# define	DEREF	271
# define	UMINUS	272


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
