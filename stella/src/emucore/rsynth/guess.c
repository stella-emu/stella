/*
    Copyright (c) 1994,2001-2002 Nick Ing-Simmons. All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "darray.h"
#include "charset.h"
#include "text.h"
#include "phones.h"
#include "lang.h"
#include "english.h"
#include "say.h"


lang_t *lang = &English;

unsigned
xlate_string(char *string, darray_ptr phone)
{
 abort(); /* just a stub */
}

int main(int argc,char *argv[])
{
 darray_t phone;
 darray_init(&phone, sizeof(char), 128);
 init_locale();
 if (argc > 1)
  {
   rule_debug = 1;
   while (--argc)
    {
     char *s = *++argv;
     NRL(s,strlen(s),&phone);
     darray_append(&phone,0);
     printf("%s [%s]\n",s,(char *) darray_find(&phone,0));
     phone.items = 0;
    }
  }
 else
  {
   char buf[1024];
   unsigned char *s;
   unsigned total = 0;
   unsigned right = 0;
   while ((s = (unsigned char *) fgets(buf,sizeof(buf),stdin)))
    {
     while (*s && isspace(*s)) s++;
     if (*s)
      {
       unsigned char *e = s;
       unsigned char *p;
       while (*e && !isspace(*e)) e++;
       NRL(s,(e-s),&phone);
       darray_append(&phone,0);
       p = e;
       while (*p && isspace(*p)) p++;
       if (*p)
        {
         char *q = p;
         total++;
         while (*q && !isspace(*q)) q++;
         *q++ = ' ';
         *q  = '\0';
         if (strcmp(p,darray_find(&phone,0)) != 0)
          {
           printf("%.*s",(e-s),s);
           printf(" expected /%s/, got /%s/\n",p,(char *) darray_find(&phone,0));
          }
         else
          {
           right++;
          }
        }
       else
        {
         printf("%.*s",(e-s),s);
         printf(" got /%s/\n",(char *) darray_find(&phone,0));
         fflush(stdout);
        }
       phone.items = 0;
      }
    }
   if (total)
    {
     fprintf(stderr,"%.3g%% correct\n",right*100.0/total);
    }
  }
 return 0;
}


