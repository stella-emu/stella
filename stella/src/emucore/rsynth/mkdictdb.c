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
#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include "useconfig.h"
#include "trie.h"
#include "darray.h"
#include "phones.h"
#include "charset.h"             /* national differences ,jms */
#include "dbif.h"

trie_ptr phones;

static void enter_phones(void);
static void
enter_phones(void)
{
 int i;
 char *s;
 for (i = 1; (s = ph_name[i]); i++)
  trie_insert(&phones, s, (void *) i);
}


static inline int
Delete(unsigned char *p)
{
 int ch = *p;
 while (*p)
  {
   *p = p[1];
   p++;
  }
 return ch;
}

static void enter_words(DB_HANDLE db, FILE * f)
{
 char buf[4096];
 unsigned words = 0;
 while (fgets(buf, sizeof(buf), f))
  {
   char *s = buf;
   char *h = strchr(s, '#');
   if (h)
    *h = '\0';
   while (isspace((unsigned char)*s))
    s++;
   if (*s)
    {
     unsigned char *p = (unsigned char *) s;
     while (!isspace(*p))
      {
       if (*p == '"' || *p == '~' || *p == '<')
        {
         unsigned char *x = p;
         *x = accent(x[0], x[1]);
         x++;
         strcpy((char *)x,(char *)x+1);
        }
       else if (*p == '\\')
        {
         unsigned char *x = p;
         *x = accent(x[1], x[2]);
         x++;
         strcpy((char *) x,(char *)x+2);
        }
       if (isupper(*p))
        *p = tolower(*p);
       else if (!isalpha(*p) && *p != '\'' && *p != '-' && *p != '_' && *p != '.')
        break;
       p++;
      }
     if (isspace(*p))
      {
       char codes[4096];
       char *d = codes;
       int ok = 1;
       int stress = 0;
       DATUM key;
       DATUM_SET(key, s, (char *) p - s);
       if (++words % 10000 == 0)
        {
         printf("%d %.*s\n",words, (int) DATUM_SIZE(key), (char *) DATUM_DATA(key));
        }
       while (*p && ok)
        {
         unsigned code;
         while (isspace(*p))
          p++;
         if (*p)
          {
           unsigned char *e = p;
           while (isalpha(*e) || *e == '1' || *e == '2')
            {
             if (*e == '1' || *e == '2')
              stress = 1;
             else if (islower(*e))
              *e = toupper(*e);
             e++;
            }
           if (*e == '0')
            *e++ = ' ';
           if (e > p && (code = (unsigned) trie_lookup(&phones, (char **) &p)))
            *d++ = code;
           else
            {
             fprintf(stderr, "Bad code %.*s>%s", (int)((char *) p - s), s, p);
             ok = 0;
             break;
            }
          }
        }
       if (ok)
        {
         DATUM data;
         DATUM_SET(data, codes, d - codes);
         DB_STORE(db, key, data);
        }
      }
     else
      {
       if (*p != '(')
        fprintf(stderr, "Ignore (%c) %s", *p, s);
      }
    }
  }
}

int main(int argc, char *argv[], char *env[]);

int
main(int argc, char **argv, char **envp)
{
 init_locale();
 if (argc == 3)
  {
   FILE *f = fopen(argv[1], "r");
   if (f)
    {
     DB_HANDLE db;
     DB_OPEN_WRITE(db,argv[2]);
     if (db)
      {
       enter_phones();
       enter_words(db, f);
       DB_CLOSE(db);
      }
     else
      perror(argv[2]);
     fclose(f);
    }
   else
    perror(argv[1]);
  }
 return 0;
}
