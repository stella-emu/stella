#ifndef _WCE_MISSING_H_
#define _WCE_MISSING_H_

#include <windows.h>

/* WCE 300 does not support exception handling well */
#define try	if (1)
#define catch(a) else if (0)
extern char *msg;
#define throw //

#pragma warning(disable: 4800)
#pragma warning(disable: 4244)
#pragma warning(disable: 4786)

typedef unsigned int  uintptr_t;

int time(int dummy);
char *getcwd(void);


#define MAX_KEYS 8
enum key {K_UP = 0, K_DOWN, K_LEFT, K_RIGHT, K_FIRE, K_RESET, K_SELECT, K_QUIT};

#endif
