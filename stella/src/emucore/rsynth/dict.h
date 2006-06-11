/* $Id: dict.h,v 1.1 2006-06-11 07:13:24 urchlay Exp $
*/
#ifndef DICT_H
#define DICT_H

#include "lang.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *dict_path;

extern int dict_init(const char *dictname);
extern void dict_term(void);

#ifdef __cplusplus
}
#endif

#endif /* DICT_H */
