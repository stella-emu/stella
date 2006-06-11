extern short		_u2l[];		/* 8-bit u-law to 16-bit PCM */
extern unsigned char	*_l2u;		/* 13-bit PCM to 8-bit u-law */
#define	ulaw2short(X)	(_u2l[(unsigned char) (X)])
#define	short2ulaw(X)	(_l2u[((short)(X)) >> 3])

