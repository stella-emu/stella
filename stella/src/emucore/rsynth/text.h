/*
    Copyright (c) 1994,2001-2003 Nick Ing-Simmons. All rights reserved.
 
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
/* $Id: text.h,v 1.1 2006-06-11 07:13:27 urchlay Exp $
*/
#ifndef TEXT_H
#define TEXT_H
#ifdef __cplusplus
extern "C" {
#endif

extern int rule_debug;
typedef void (*out_p)(void *arg,char *s);
extern int NRL(char *s,unsigned n,darray_ptr phone);
extern void guess_word(void *arg, out_p out, unsigned char *word);

#ifdef __cplusplus
}
#endif

#endif /* TEXT_H */
