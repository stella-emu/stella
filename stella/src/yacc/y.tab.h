#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

#ifndef YYSTYPE
typedef union {
	int val;
	char *equate;
	CPUDEBUG_INT_METHOD cpuMethod;
	TIADEBUG_INT_METHOD tiaMethod;
	Expression *exp;
	char *function;
} yystype;
# define YYSTYPE yystype
# define YYSTYPE_IS_TRIVIAL 1
#endif
# define	NUMBER	257
# define	ERR	258
# define	EQUATE	259
# define	CPU_METHOD	260
# define	TIA_METHOD	261
# define	FUNCTION	262
# define	LOG_OR	263
# define	LOG_AND	264
# define	LOG_NOT	265
# define	SHR	266
# define	SHL	267
# define	GTE	268
# define	LTE	269
# define	NE	270
# define	EQ	271
# define	DEREF	272
# define	UMINUS	273


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
