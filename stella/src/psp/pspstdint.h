/* SCE CONFIDENTIAL
 PSP(TM) Programmer Tool Runtime Library Release 1.5.0
 *
 *      Copyright (C) 2005 Sony Computer Entertainment Inc.
 *                        All Rights Reserved.
 *
 */
/*
 *
 *                   PSP(TM) integer types
 *
 *                        pspstdint.h
 *
 *       Version        Date            Design      Log
 *  --------------------------------------------------------------------
 *      0.00          2005-01-19        kono        the first version
 */


#ifndef _SCE_PSPSTDINT_H
#define _SCE_PSPSTDINT_H

/* Exact-width integer types */
#ifndef _SCE_PSPSTDINT_int8_t_DEFINED
#define _SCE_PSPSTDINT_int8_t_DEFINED
typedef signed char			int8_t;
typedef unsigned char		uint8_t;
typedef short				int16_t;
typedef unsigned short		uint16_t;
typedef int					int32_t;
typedef unsigned int		uint32_t;
#if defined(__GNUC__)
__extension__ typedef long long				int64_t   __attribute__((mode(DI)));
__extension__ typedef unsigned long long	uint64_t  __attribute__((mode(DI)));
#else	/* defined(__GNUC__) */
typedef long long			int64_t;
typedef unsigned long long	uint64_t;
#endif	/* defined(__GNUC__) */
#endif	/* _SCE_PSPSTDINT_int8_t_DEFINED */


/* Minimum-width integer types */
#ifndef _SCE_PSPSTDINT_int_least8_t_DEFINED
#define	_SCE_PSPSTDINT_int_least8_t_DEFINED
typedef signed char			int_least8_t;
typedef unsigned char		uint_least8_t;
typedef short				int_least16_t;
typedef unsigned short		uint_least16_t;
typedef int					int_least32_t;
typedef unsigned int		uint_least32_t;
#if defined(__GNUC__)
__extension__ typedef long long				int_least64_t   __attribute__((mode(DI)));
__extension__ typedef unsigned long long	uint_least64_t  __attribute__((mode(DI)));
#else	/* defined(__GNUC__) */
typedef long long			int_least64_t;
typedef unsigned long long	uint_least64_t;
#endif	/* defined(__GNUC__) */
#endif	/* _SCE_PSPSTDINT_int_least8_t_DEFINED */


/* Fastest minimum-width integer types */
#ifndef _SCE_PSPSTDINT_int_fast8_t_DEFINED
#define	_SCE_PSPSTDINT_int_fast8_t_DEFINED
typedef char				int_fast8_t;
typedef unsigned char		uint_fast8_t;
typedef int					int_fast16_t;
typedef unsigned int		uint_fast16_t;
typedef int					int_fast32_t;
typedef unsigned int		uint_fast32_t;
#if defined(__GNUC__)
__extension__ typedef long long				int_fast64_t   __attribute__((mode(DI)));
__extension__ typedef unsigned long long	uint_fast64_t  __attribute__((mode(DI)));
#else	/* defined(__GNUC__) */
typedef long long			int_fast64_t;
typedef unsigned long long	uint_fast64_t;
#endif	/* defined(__GNUC__) */
#endif	/* _SCE_PSPSTDINT_int_fast8_t_DEFINED */


/* Integer types capable of holding object pointers */
#ifndef _SCE_PSPSTDINT_intptr_t_DEFINED
#define _SCE_PSPSTDINT_intptr_t_DEFINED
typedef int					intptr_t;
typedef unsigned int		uintptr_t;
#endif	/* _SCE_PSPSTDINT_intptr_t_DEFINED */


/* Gereat-width integer types */
#ifndef _SCE_PSPSTDINT_intmax_t_DEFINED
#define _SCE_PSPSTDINT_intmax_t_DEFINED
#if defined(__GNUC__)
typedef long long			intmax_t   __attribute__((mode(DI)));
typedef unsigned long long	uintmax_t  __attribute__((mode(DI)));
#else	/* defined(__GNUC__) */
typedef long long			intmax_t;
typedef unsigned long long	uintmax_t;
#endif	/* defined(__GNUC__) */
#endif	/* _SCE_PSPSTDINT_intmax_t_DEFINED */


/* Limits of specified-width intger types */
#if (!(defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)))||defined(__STDC_CONSTANT_MACROS)

/* Limits of exact-width integer types */
#define INT8_MIN			(-128)
#define INT16_MIN			(-32767-1)
#define INT32_MIN			(-2147483647-1)
#define INT64_MIN			(-9223372036854775807LL-1)
#define INT8_MAX			(127)
#define INT16_MAX			(32767)
#define INT32_MAX			(2147483647)
#define INT64_MAX			(9223372036854775807LL)
#define UINT8_MAX			(255)
#define UINT16_MAX			(65535)
#define UINT32_MAX			(4294967295U)
#define UINT64_MAX			(18446744073709551615ULL)

/* Limits of minimum-width integer types */
#define INT_LEAST8_MIN		(-128)
#define INT_LEAST16_MIN		(-32767-1)
#define INT_LEAST32_MIN		(-2147483647-1)
#define INT_LEAST64_MIN		(-9223372036854775807LL-1)
#define INT_LEAST8_MAX		(127)
#define INT_LEAST16_MAX		(32767)
#define INT_LEAST32_MAX		(2147483647)
#define INT_LEAST64_MAX		(9223372036854775807LL)
#define UINT_LEAST8_MAX		(255)
#define UINT_LEAST16_MAX	(65535)
#define UINT_LEAST32_MAX	(4294967295U)
#define UINT_LEAST64_MAX	(18446744073709551615ULL)

/* Limits of fastest minimum-width integer types */
#define INT_FAST8_MIN		(-128)
#define INT_FAST16_MIN		(-2147483647-1)
#define INT_FAST32_MIN		(-2147483647-1)
#define INT_FAST64_MIN		(-9223372036854775807LL-1)
#define INT_FAST8_MAX		(127)
#define INT_FAST16_MAX		(2147483647)
#define INT_FAST32_MAX		(2147483647)
#define INT_FAST64_MAX		(9223372036854775807LL)
#define UINT_FAST8_MAX		(255)
#define UINT_FAST16_MAX		(4294967295U)
#define UINT_FAST32_MAX		(4294967295U)
#define UINT_FAST64_MAX		(18446744073709551615ULL)

/* Limits of integer types capable of holding object pointers */
#define INTPTR_MIN			(-2147483647-1)
#define INTPTR_MAX			(2147483647)
#define UINTPTR_MAX			(4294967295U)

/* Limits of greates-width intger types */
#define INTMAX_MIN			(-9223372036854775807LL-1)
#define INTMAX_MAX			(9223372036854775807LL)
#define UINTMAX_MAX			(18446744073709551615ULL)


/* Macros for minimum-width integer constants */
#define INT8_C(c)			c
#define INT16_C(c)			c
#define INT32_C(c)			c
#define INT64_C(c)			c ## LL
#define UINT8_C(c)			c ## U
#define UINT16_C(c)			c ## U
#define UINT32_C(c)			c ## U
#define UINT64_C(c)			c ## ULL

/* Macros for greatest-width integer constants */
#define INTMAX_C(c)			c ## LL
#define UINTMAX_C(c)		c ## ULL


#endif	/* (!(defined(_LANGUAGE_C_PLUS_PLUS)||defined(__cplusplus)||defined(c_plusplus)))||defined(__STDC_CONSTANT_MACROS) */

#endif  /* _SCE_PSPSTDINT_H */

