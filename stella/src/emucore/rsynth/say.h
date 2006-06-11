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
/* $Id: say.h,v 1.1 2006-06-11 07:13:27 urchlay Exp $
*/
#ifndef SAY_H
#define SAY_H 
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Routines that do text -> phonetic conversion */
extern unsigned xlate_string(char *string,darray_ptr phone);
extern int suspect_word(char *s,int n);
extern unsigned spell_out(char *word,int n,darray_ptr phone);


/* text -> phonetic -> elements -> sampled audio  */
struct rsynth_s;
extern void say_string(struct rsynth_s *rsynth, char *s);
extern void say_file(struct rsynth_s *rsynth, FILE * f);

#ifdef __cplusplus
}
#endif


#endif /* SAY_H */


