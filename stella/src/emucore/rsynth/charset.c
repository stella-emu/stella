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
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include "charset.h"

void
init_locale(void)
{
 char *s = setlocale(LC_ALL, "");
 if (s)
  {
   s = setlocale(LC_CTYPE, NULL);
   if (s && !strcmp(s, "C"))
    {
     s = setlocale(LC_CTYPE, "iso_8859_1");
     if (!s)
      s = setlocale(LC_CTYPE, "C");
    }
  }
}

int
deaccent(int ch)
{
 /* Cast to char (type of litterals) as signedness may differ */
 switch ((char)ch)
  {
   case 'à':
   case 'á':
   case 'â':
   case 'ã':
   case 'ä':
   case 'å':
    return 'a';
   case 'ç':
    return 'c';
    break;
   case 'è':
   case 'é':
   case 'ê':
   case 'ë':
    return 'e';
   case 'ì':
   case 'í':
   case 'î':
   case 'ï':
    return 'i';
   case 'ñ':
    return 'n';
   case 'ò':
   case 'ó':
   case 'ô':
   case 'õ':
   case 'ö':
    return 'o';
   case 'ù':
   case 'ú':
   case 'û':
   case 'ü':
    return 'u';
   case 'ý':
   case 'ÿ':
    return 'y';
   case 'À':
   case 'Á':
   case 'Â':
   case 'Ã':
   case 'Ä':
   case 'Å':
    return 'A';
   case 'Ç':
    return 'C';
   case 'È':
   case 'É':
   case 'Ê':
   case 'Ë':
    return 'E';
   case 'Ì':
   case 'Í':
   case 'Î':
   case 'Ï':
    return 'I';
   case 'Ñ':
    return 'N';
   case 'Ò':
   case 'Ó':
   case 'Ô':
   case 'Õ':
   case 'Ö':
    return 'O';
   case 'Ù':
   case 'Ú':
   case 'Û':
   case 'Ü':
    return 'U';
   case 'Ý':
    return 'Y';
  }
 if ((ch & 0xFF) > 0x7F)
  abort();
 return ch;
}

int
accent(int a, int c)
{
 if (a && c)
  {
   switch (a)
    {
     case '<':
     case ',':
      switch (c)
       {
        case 'c':
         return 'ç';
        case 'C':
         return 'Ç';
        default:
         return c;
       }
     case '~':
      switch (c)
       {
        case 'n':
         return 'ñ';
        case 'a':
         return 'ã';
        case 'o':
         return 'õ';
        case 'N':
         return 'Ñ';
        case 'A':
         return 'Ã';
        case 'O':
         return 'Õ';
        default:
         return c;
       }
     case '\'':
      switch (c)
       {
        case 'a':
         return 'á';
        case 'e':
         return 'é';
        case 'i':
         return 'í';
        case 'o':
         return 'ó';
        case 'u':
         return 'ú';
        case 'y':
         return 'ý';
        case 'A':
         return 'Á';
        case 'E':
         return 'É';
        case 'I':
         return 'Í';
        case 'O':
         return 'Ó';
        case 'U':
         return 'Ú';
        case 'Y':
         return 'Ý';
        default:
         return c;
       }
     case '`':
      switch (c)
       {
        case 'a':
         return 'à';
        case 'e':
         return 'è';
        case 'i':
         return 'ì';
        case 'o':
         return 'ò';
        case 'u':
         return 'ù';
        case 'A':
         return 'À';
        case 'E':
         return 'È';
        case 'I':
         return 'Ì';
        case 'O':
         return 'Ò';
        case 'U':
         return 'Ù';
        default:
         return c;
       }
     case '^':
      switch (c)
       {
        case 'a':
         return 'â';
        case 'e':
         return 'ê';
        case 'i':
         return 'î';
        case 'o':
         return 'ô';
        case 'u':
         return 'û';
        case 'A':
         return 'Â';
        case 'E':
         return 'Ê';
        case 'I':
         return 'Î';
        case 'O':
         return 'Ô';
        case 'U':
         return 'Û';
        default:
         return c;
       }
     case '"':
      switch (c)
       {
        case 'a':
         return 'ä';
        case 'e':
         return 'ë';
        case 'i':
         return 'ï';
        case 'o':
         return 'ö';
        case 'u':
         return 'ü';
        case 'y':
         return 'ÿ';
        case 'A':
         return 'Ä';
        case 'E':
         return 'Ë';
        case 'I':
         return 'Ï';
        case 'O':
         return 'Ö';
        case 'U':
         return 'Ü';
        default:
         return c;
       }
    }
  }
 return c;
}
