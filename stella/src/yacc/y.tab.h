#ifndef BISON_Y_TAB_H
# define BISON_Y_TAB_H

# ifndef YYSTYPE
#  define YYSTYPE int
#  define YYSTYPE_IS_TRIVIAL 1
# endif
# define	NUMBER	257
# define	LOG_OR	258
# define	LOG_AND	259
# define	SHR	260
# define	SHL	261
# define	UMINUS	262
# define	GTE	263
# define	LTE	264
# define	NE	265
# define	EQ	266


extern YYSTYPE yylval;

#endif /* not BISON_Y_TAB_H */
