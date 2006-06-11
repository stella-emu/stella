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
/* $Id: hplay.h,v 1.2 2006-06-11 21:49:08 stephena Exp $
*/
#ifndef __HPLAY_H
#define __HPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

extern char *program;
extern long samp_rate;
extern int audio_init(int argc, char *argv[]);
extern void audio_term(void);
extern void audio_play(int n, short *data);

#ifdef __cplusplus
}
#endif

#endif
