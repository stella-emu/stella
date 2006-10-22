//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2005 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: scale3x.hxx,v 1.1 2006-10-22 18:58:45 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2001-2006 The ScummVM project
//============================================================================

/*
 * This file is part of the Scale2x project.
 *
 * Copyright (C) 2001, 2002, 2003, 2004 Andrea Mazzoleni
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __SCALE3X_H
#define __SCALE3X_H

#if defined(_MSC_VER)
#define __restrict__
#endif

typedef unsigned char scale3x_uint8;
typedef unsigned short scale3x_uint16;
typedef unsigned scale3x_uint32;

void scale3x_8_def(scale3x_uint8* dst0, scale3x_uint8* dst1, scale3x_uint8* dst2, const scale3x_uint8* src0, const scale3x_uint8* src1, const scale3x_uint8* src2, unsigned count);
void scale3x_16_def(scale3x_uint16* dst0, scale3x_uint16* dst1, scale3x_uint16* dst2, const scale3x_uint16* src0, const scale3x_uint16* src1, const scale3x_uint16* src2, unsigned count);
void scale3x_32_def(scale3x_uint32* dst0, scale3x_uint32* dst1, scale3x_uint32* dst2, const scale3x_uint32* src0, const scale3x_uint32* src1, const scale3x_uint32* src2, unsigned count);

#endif
