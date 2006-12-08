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
// Copyright (c) 1995-2006 by Bradford W. Mott and the Stella team
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: scaler.hxx,v 1.2 2006-12-08 16:48:58 stephena Exp $
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2001-2006 The ScummVM project
//============================================================================

#ifndef GRAPHICS_SCALER_H
#define GRAPHICS_SCALER_H

//#include "common/stdafx.h"
#include "compat.hxx"
//#include "graphics/surface.h"

extern void InitScalers();
extern void FreeScalers();

typedef void ScalerProc(const uint8 *srcPtr, uint32 srcPitch,
                        uint8 *dstPtr, uint32 dstPitch,
                        int width, int height);

#define DECLARE_SCALER(x)	\
	extern void x(const uint8 *srcPtr, uint32 srcPitch, uint8 *dstPtr, \
					uint32 dstPitch, int width, int height)

DECLARE_SCALER(AdvMame2x);
DECLARE_SCALER(AdvMame3x);
DECLARE_SCALER(HQ2x);
DECLARE_SCALER(HQ3x);

#endif
