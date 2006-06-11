/*
    Copyright (c) 1994,2001-2002 Nick Ing-Simmons. All rights reserved.
 
    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
    MA 02111-1307, USA

*/
#include <config.h>
/* $Id: getarg.c,v 1.1 2006-06-11 07:13:25 urchlay Exp $
 */
char *getarg_id = "$Id: getarg.c,v 1.1 2006-06-11 07:13:25 urchlay Exp $";
#if defined(USE_PROTOTYPES) ? USE_PROTOTYPES : defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdio.h>
#include <ctype.h>
#include <useconfig.h>
#include "getargs.h"

int help_only = 0;

/*
   Usage is like :

   argc = getargs("Module",argc,argv,
   "r", "%d", &rate,               "Integer",
   "u", "",   &ufile_name,         "String",
   "g", "%d", &gain,               "Double",
   "a", NULL, &use_audio,          "Boolean -a toggles, +a sets",
   "s", ""  , &speaker,            "Boolean -s clears, +s sets",
   NULL);

 */


#if defined(USE_PROTOTYPES) ? USE_PROTOTYPES : defined(__STDC__)
int
getargs(char *module,int argc, char *argv[],...)
#else
int
getargs(module, argc, argv, va_alist)
char *module;
int argc;
char *argv[];
va_dcl
#endif
{
 va_list ap;
 int i = 0;
 int done_module = 0;
 while (i < argc)
  {
   char *s = argv[i];
   int flag = *s++;
   if (*s && (flag == '-' || flag == '+'))
    {
     int off = 1;
     while (*s)
      {
       char *a;
       int count = 0;
#if defined(USE_PROTOTYPES) ? USE_PROTOTYPES : defined(__STDC__)
       va_start(ap, argv);
#else
       va_start(ap);
#endif
       while ((a = va_arg(ap, char *)))
        {
         int l = strlen(a);
         char *fmt = va_arg(ap, char *);
         void *var = va_arg(ap, void *);
         char *desc = va_arg(ap, char *);
         if (!strcmp(s,"-help"))
          {
           help_only = 1;
           if (!done_module++)
            fprintf(stderr,"%s:\n",module);
           if (fmt)
            {char *x = strchr(fmt,'%');
             if (x)
              {
               fprintf(stderr," -%s <%s> [",a,x+1);
               switch(*(x+strlen(x)-1))
                {
                 case 'e':
                 case 'f':
                 case 'g':
                  fprintf(stderr,fmt,*((double *) var));
                  break;
                 case 'u':
                 case 'd':
                  if (x[1] == 'l')
                   fprintf(stderr,fmt,*((long *) var));
                  else
                   fprintf(stderr,fmt,*((int *) var));
                  break;
                }
               fprintf(stderr,"]\t%s\n",desc);
              }
             else
              fprintf(stderr," -%s <string> [%s]\t%s\n",a,
                      *((char **) var) ? *((char **) var) : "", desc);
            }
           else
            {
             fprintf(stderr," [+|-]%s [%s]\t%s\n",a,
                     *((int *) var) ? "yes" : "no",desc);
            }
          }
         else if (l > 1)
          {
           if (!count && !strcmp(s, a))
            {
             if (fmt)
              {
               if (i + off < argc)
                {
                 char *x = argv[i + off++];
                 if (strchr(fmt, '%'))
                  {
                   if (sscanf(x, fmt, var) != 1)
                    fprintf(stderr, "%s : %s invalid after -%s\n", argv[0], x, a);
                  }
                 else
                  *((char **) var) = x;
                }
               else
                fprintf(stderr, "%s : no argument after -%s\n", argv[0], a);
              }
             else
              {
               if (flag == '+')
                *((int *) var) = !0;
               else
                *((int *) var) = !*((int *) var);
              }
             /* skip to end of string */
             count++;
             s += l;
             break;               /* out of va_arg loop */
            }
          }
         else
          {
           if (*s == *a)
            {
             if (fmt)
              {
               if (i + off < argc)
                {
                 char *x = argv[i + off++];
                 if (strchr(fmt, '%'))
                  {
                   if (sscanf(x, fmt, var) != 1)
                    fprintf(stderr, "%s : %s invalid after -%s\n", argv[0], x, a);
                  }
                 else
                  *((char **) var) = x;
                }
               else
                fprintf(stderr, "%s : no argument after -%s\n", argv[0], a);
              }
             else
              {
               if (fmt || flag == '+')
                *((int *) var) = (flag == '+');
               else
                *((int *) var) = !*((int *) var);
              }
             count++;
             s++;
             break;               /* out of va_arg loop */
            }
          }
        }
       va_end(ap);
       if (!count)
        {
         off = 0;
         break;                   /* Out of s loop */
        }
      }
     if (off != 0)
      {
       int j;
       argc -= off;
       for (j = i; j <= argc; j++)
        argv[j] = argv[j + off];
      }
     else
      i++;
    }
   else
    i++;
  }
 if (help_only && done_module)
  fprintf(stderr,"\n");
 return argc;
}
