/*****************************************************************************/
/*                                                                           */
/* Module:  SBDRV.H                                                          */
/* Purpose: Define function prototypes and structures required for the       */
/*          SB DRV routines, V1.3.                                           */
/* Author(s): Ron Fries and Neil Bradley                                     */
/*                                                                           */
/* 01/30/97 - Initial Release                                                */
/* 08/24/97 - V1.1 - Added defintion of SBDRV_SHOW_ERR to cause the SBDRV    */
/*                   to display error messages.  Comment line to supress     */
/* 01/12/98 - V1.2 - Added support for DJGPP.                                */
/* 02/04/99 - V1.3 - Cleaned up DJGPP support, fixed a possible segfault     */
/*                   in the reading of the BLASTER= env. variable, and       */
/*                   added timeout to dsp_out().                             */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* SBDrv is Copyright(c) 1997-1999 by Ron Fries, Neil Bradley and            */
/*       Bradford Mott                                                       */
/*                                                                           */
/* This library is free software; you can redistribute it and/or modify it   */
/* under the terms of version 2 of the GNU Library General Public License    */
/* as published by the Free Software Foundation.                             */
/*                                                                           */
/* This library is distributed in the hope that it will be useful, but       */
/* WITHOUT ANY WARRANTY; without even the implied warranty of                */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library */
/* General Public License for more details.                                  */
/* To obtain a copy of the GNU Library General Public License, write to the  */
/* Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.   */
/*                                                                           */
/* Any permitted reproduction of these routines, in whole or in part, must   */
/* bear this legend.                                                         */
/*                                                                           */
/*****************************************************************************/

#ifndef _SBDRV_H
#define _SBDRV_H

#ifndef _TYPEDEF_H
#define _TYPEDEF_H

#define SBDRV_SHOW_ERR      /* delete line to supress error message printing */

/* define some data types to keep it platform independent */
#ifdef COMP16                 /* if 16-bit compiler defined */
#define int8  char
#define int16 int
#define int32 long
#else                         /* else default to 32-bit compiler */
#define int8  char
#define int16 short
#define int32 int
#endif

#define uint8  unsigned int8
#define uint16 unsigned int16
#define uint32 unsigned int32

#endif

/* CONSTANT DEFINITIONS */

#define AUTO_DMA      0      /* selects auto-initialize DMA mode */
#define STANDARD_DMA  1      /* selects standard DMA mode */

/* global function prototypes */

#ifdef __cplusplus
extern "C" {
#endif

uint8 OpenSB(uint16 playback_freq, uint16 buffer_size);
void CloseSB(void);
uint8 Start_audio_output (uint8 dma_mode,
                          void (*fillBuffer)(uint8 *buf,uint16 n));
void Stop_audio_output (void);

void Set_master_volume(uint8 left, uint8 right);
void Set_line_volume(uint8 left, uint8 right);
void Set_FM_volume(uint8 left, uint8 right);

#ifdef __cplusplus
}
#endif

#endif
