/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
** dos_sb.h
**
** DOS Sound Blaster header file
** $Id: dos_sb.h,v 1.1 2002-11-13 03:47:55 bwmott Exp $
*/

#ifndef DOS_SB_H
#define DOS_SB_H

/* Thanks, Allegro! */
#define  BPS_TO_TIMER(x)            (1193182L / (long)(x))
#define  END_OF_FUNCTION(x)         void x##_end(void) {}
#define  END_OF_STATIC_FUNCTION(x)  static void x##_end(void) {}
#define  LOCK_VARIABLE(x)           _go32_dpmi_lock_data((void*)&x, sizeof(x))
#define  LOCK_FUNCTION(x)           _go32_dpmi_lock_code(x, (long)x##_end - (long)x)

#define  DISABLE_INTS()             __asm__ __volatile__ ("cli")
#define  ENABLE_INTS()              __asm__ __volatile__ ("sti")

typedef void (*sbmix_t)(void *userdata, void *buffer, int size);

#ifdef __cplusplus
extern "C" {
#endif

extern int  sb_init(int *sample_rate, int *bps, int *buf_size, int *stereo);
extern void sb_shutdown(void);
extern int  sb_startoutput(sbmix_t fillbuf, void *userdata);
extern void sb_stopoutput(void);
extern void sb_setrate(int rate);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DOS_SB_H */

