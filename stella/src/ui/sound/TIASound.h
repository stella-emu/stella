/*****************************************************************************/
/*                                                                           */
/* Module:  TIA Chip Sound Simulator Includes, V1.1                          */
/* Purpose: Define global function prototypes and structures for the TIA     */
/*          Chip Sound Simulator.                                            */
/* Author:  Ron Fries                                                        */
/*                                                                           */
/* Revision History:                                                         */
/*    10-Sep-96 - V1.0 - Initial Release                                     */
/*    14-Jan-97 - V1.1 - Added compiler directives to facilitate compilation */
/*                       on a C++ compiler.                                  */
/*                                                                           */
/*****************************************************************************/
/*                                                                           */
/*                 License Information and Copyright Notice                  */
/*                 ========================================                  */
/*                                                                           */
/* TiaSound is Copyright(c) 1997 by Ron Fries                                */
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

#ifndef _TIASOUND_H
#define _TIASOUND_H

#ifdef __cplusplus
extern "C" {
#endif

void Tia_sound_init (unsigned int sample_freq, unsigned int playback_freq);
void Update_tia_sound (unsigned int addr, unsigned char val);
void Tia_process_2 (register unsigned char *buffer,
                    register unsigned int n);
void Tia_process (register unsigned char *buffer,
                  register unsigned int n);

#ifdef __cplusplus
}
#endif

#endif
